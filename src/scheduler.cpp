#include "scheduler.h"

#include "utils/string.h"

namespace mpits {

namespace {

	void wakeup_group(const Scheduler& sched, const std::vector<int>& ranks, const Task::TaskID& tid);

	void task_spawn(Scheduler& sched);

	template <class Functor>
	inline void resume_workers(Scheduler& sched, const std::vector<int>& ranks, const Functor& func) {

		for(int idx : ranks) {
			kill(sched.pid_list()[idx-1].second, SIGCONT);
			func(idx);
		}

	}

	/**
	 * Creates a task and push it into the task queue hosted by the 
	 * scheduler 
	 */
	Task::TaskID create_task(Scheduler& 		sched, 
							 const std::string& kernel, 
							 unsigned 			min, 
							 unsigned 			max) 
	{
	
		Task::TaskID tid = sched.next_tid();

		auto task = std::make_shared<Task>(tid, kernel, min, max);
		sched.enqueue_task( task );
		
		LOG(INFO) << "Created Task: " << *task; 

		// create an event 
		sched.cmd_queue().push( Event(Event::TASK_CREATED, utils::any(std::move(tid))) );

		return tid;
	}


	/** 
	 * Handle incoming messages to the scheduler by implementeing their 
	 * semantic actions 
	 */
	bool message_dispatch(Scheduler& sched, const comm::Message& msg) {

		using namespace comm;

		switch(msg.msg_id()) {

		case Message::TASK_CREATE: 
			{

				typedef std::tuple<std::string,unsigned,unsigned> ContentType;

				auto content = msg.get_content_as<ContentType>();

				Task::TaskID tid = 
					create_task(sched, std::get<0>(content), 
								std::get<1>(content), std::get<2>(content)
							);

				MPI_Send(&tid, 1, MPI_UNSIGNED_LONG, msg.endpoint(), 0, msg.comm()); 
				break;
			}

		case Message::TASK_COMPLETED:
			/**
			 * When we receive a message from the master worker saying that the task is completed 
			 * we all generate an internal event
			 */
			{	
				auto desc = msg.get_content_as<std::tuple<Task::TaskID>>();
				
				Task::TaskID tid = std::get<0>(desc);
				auto& active_tasks = sched.active_tasks();
				auto fit = active_tasks.find(tid);
				assert(fit != active_tasks.end());

				// Make the pids available for successive tasks 
				sched.release_pids(fit->second->ranks()); 

				// Remove the task
				active_tasks.erase(fit);

				sched.cmd_queue().push(
					Event(Event::TASK_COMPLETED, utils::any(std::move(tid))) 
				);

				break;
			}

		case Message::TASK_WAIT:
			/** 
			 * A worker group is waiting for another task, the worker is paused waiting to be wake
			 * up, therefore we register an event handler to wake up the worker upon completition
			 */
			{
				auto desc = msg.get_content_as<std::tuple<Task::TaskID, Task::TaskID>>();
				LOG(INFO) << "Task '" << std::get<0>(desc) 
					      << "' waiting for tasks: " << std::get<1>(desc);
				
				Task::TaskID tid = std::get<0>(desc);

				auto& active_tasks = sched.active_tasks();
				auto fit = active_tasks.find(tid);
				assert(fit != active_tasks.end());


				auto resume_worker = 
					[&sched, tid](const Task::TaskID& cur) { 
						auto& t = sched.active_tasks()[tid];

						auto msg = [&](const int& idx) { 
							MPI_Send(const_cast<Task::TaskID*>(&tid), 1, MPI_UNSIGNED_LONG, 
									 sched.pid_list()[idx-1].first, 3, sched.node_comm());
						};

						resume_workers(sched, t->ranks(), msg);
						sleep(1);
						return true;
					};

				if (sched.is_completed(tid)) {
					resume_worker;
					break;
				}
			
				// Make the pids available for successive tasks 
				sched.release_pids(fit->second->ranks()); 

				sched.handler().connect(
						Event::TASK_COMPLETED, 
						std::function<bool (const Task::TaskID&)>(
							resume_worker		
						),
						// Filter the particular event 
						std::function<bool (const Task::TaskID&)>(
							[=](const Task::TaskID& cur) { 
								return cur == std::get<1>(desc); 
							}
						)
					);
				
				// try to schedule a new task 
				task_spawn(sched);

				break;
			}

		default:
			assert(false);
		}

		return false;
	}

	/**
	 * TaskSpawn takes care of actually activating a task 
	 */
	void task_spawn(Scheduler& sched) {
		
		auto t = sched.next_task();
		if (!t) { return; }

		LOG(DEBUG) << "Spawning task: " << *t;

		unsigned min = t->min();
		
		assert(sched.free_ranks().size() >= min);

		std::vector<int> ranks(min);
		auto it = sched.free_ranks().begin();

		for (int i=0; i<min; ++i)
			ranks[i] = *(it++);

		for (auto rank : ranks) { sched.free_ranks().erase(rank); }

		// Store the task as an Active task
		sched.active_tasks().insert( std::make_pair(t->tid(), std::make_shared<LocalTask>(*t, ranks)) );

		auto msg = [&](const int& idx) { 
			MPI_Send(const_cast<int*>(&ranks.front()), ranks.size(), 
					 MPI_INT, sched.pid_list()[idx-1].first, 1, sched.node_comm()
			);
		};

		resume_workers(sched, ranks, msg);

		// Send the TID 
		MPI_Send(const_cast<Task::TaskID*>(&t->tid()), 1, MPI_UNSIGNED_LONG, 
				 ranks.front(), 0, sched.node_comm());

		// Send the name of the function to be invoked 
		MPI_Send(const_cast<char*>(t->kernel().c_str()), t->kernel().length()+1, MPI_CHAR,
				 ranks.front(), 0, sched.node_comm());
	}


} // end anonymous namespace 

void Scheduler::do_work() {
	
	// connect handler for message_recvd 
	m_handler.connect(
			Event::MSG_RECVD, 
			std::function<bool (const comm::Message&)>(
				std::bind(message_dispatch, std::ref(*this), std::placeholders::_1)
			)
		);

	// connect handler for task_created 
	m_handler.connect(
			Event::TASK_CREATED, 
			std::function<bool (const Task::TaskID&)>(
				[&](const Task::TaskID& cur) { 
					task_spawn(*this);
					return false;
				}
			)
		);

	m_handler.queue().push( 
		Event(Event::RECV_CHN_PROBE, 
			  utils::any(10ul, std::vector<MPI_Comm>({node_comm()}))) 
		);
}

Task::TaskID Scheduler::spawn(const std::string& kernel, unsigned min, unsigned max) {
	return create_task(*this, kernel, min, max);
}

void Scheduler::wait_for(const Task::TaskID& tid) {

	std::mutex m;
	std::condition_variable cond_var;

	m_handler.connect(
			Event::TASK_COMPLETED, 
			std::function<bool (const Task::TaskID&)>(
				[&](const Task::TaskID& cur) { 
					cond_var.notify_one();
					return true;
				}
			),
			// Filter the particular event 
			std::function<bool (const Task::TaskID&)>(
				[&](const Task::TaskID& cur) { return cur == tid; }
			)
		);
	
	std::unique_lock<std::mutex> lock(m);
	cond_var.wait(lock);

}

void Scheduler::finalize() { 

	for(auto& idxs : pid_list()) {
		kill(idxs.second, SIGCONT);
		MPI_Send(NULL, 0, MPI_BYTE, idxs.first, 0, node_comm());
	}

	cmd_queue().push( Event(Event::SHUTDOWN,true)  );

	join();

	MPI_Finalize();
}


} // end mpits namespace 
