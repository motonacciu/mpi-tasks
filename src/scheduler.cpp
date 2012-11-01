#include "scheduler.h"

namespace mpits {

namespace {

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
		default:
			assert(false);
		}

		return false;
	}


	void make_group(const Scheduler& sched, const std::vector<int>& ranks) {
			
		for(int idx : ranks) {
			kill(sched.pid_list()[idx-1].second, SIGCONT);
			MPI_Send(const_cast<int*>(&ranks.front()), ranks.size(), 
					 MPI_INT, sched.pid_list()[idx-1].first, 1, sched.node_comm()
			);
		}
	}

	/**
	 * TaskSpawn takes care of actually activating a task 
	 */
	void task_spawn(Scheduler& sched) {
		
		std::shared_ptr<Task> t = sched.next_task();
		LOG(INFO) << "Spawning task: " << *t;

		unsigned min = t->min();
		make_group(sched, {1,2});

		MPI_Send(const_cast<char*>(t->kernel().c_str()), t->kernel().length()+1, MPI_CHAR,
				 1, 0, sched.node_comm());
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

} // end mpits namespace 
