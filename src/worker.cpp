
#include "worker.h"

#include "comm/message.h"
#include "comm/channel.h"

#include "utils/logging.h"
#include "utils/string.h"

#include <dlfcn.h>

#include <thread>

#include <boost/context/all.hpp>

namespace ctx = boost::context;


namespace mpits {

	struct TaskDesc {

		Task::TaskID					m_tid;
		MPI_Comm 						m_comm; 
		ctx::fcontext_t* 				m_ctx_ptr;
		void*							m_stack_ptr;
		ctx::guarded_stack_allocator& 	m_alloc;

		TaskDesc(const TaskDesc&) = delete;
		TaskDesc& operator=(const TaskDesc&) = delete;

		TaskDesc(const Task::TaskID& 			tid, 
				 const MPI_Comm& 				comm, 
				 ctx::fcontext_t* 				ctx_ptr, 
				 void*							stack_ptr,
				 ctx::guarded_stack_allocator&  alloc) : 
			m_tid(tid), m_comm(comm), m_ctx_ptr(ctx_ptr), 
			m_stack_ptr(stack_ptr), m_alloc(alloc) { }

		const MPI_Comm& comm() const { return m_comm; }

		const Task::TaskID& tid() const { return m_tid; }

		ctx::fcontext_t* ctx() const { return m_ctx_ptr; }

		~TaskDesc() {
			MPI_Comm_free(&m_comm);
			m_alloc.deallocate(m_stack_ptr, ctx::guarded_stack_allocator::minimum_stacksize());
		}
	
	};

	typedef std::unique_ptr<TaskDesc> TaskDescPtr;

} // end mpits namespace 


typedef std::map<mpits::Task::TaskID, mpits::TaskDescPtr> ActiveTaskList;

ActiveTaskList 				active_tasks;
ActiveTaskList::iterator 	curr_active_task = active_tasks.end();

std::vector<mpits::TaskDescPtr> ctx_clean;

namespace {

	using namespace mpits; 

	/**
	 * taken the idea from:
	 *  	https://svn.mcs.anl.gov/repos/mpi/mpich2/trunk/test/mpi/spawn/pgroup_intercomm_test.c
	 */
	MPI_Comm make_group(const mpits::Worker& worker, const std::vector<int>& ranks) {
		
		/* CASE: Group size 0 */
		assert (!ranks.empty() );

		/* CASE: Group size 1 */
		if (ranks.size() == 1 && ranks.front() == worker.node_rank()) { return MPI_COMM_SELF; }


		auto fit = std::find(ranks.begin(), ranks.end(), worker.node_rank());
		assert(fit != ranks.end());

		size_t idx = std::distance(ranks.begin(), fit);

		MPI_Comm pgroup = MPI_COMM_SELF;
		for(unsigned merge_size = 1, size=ranks.size(); merge_size < size; merge_size *= 2) {
		   
			unsigned gid = idx / merge_size;
			MPI_Comm pgroup_old = pgroup, inter_pgroup;

			if (gid % 2 == 0) {

				/* Check if right partner doesn't exist */
				if ((gid+1)*merge_size >= ranks.size()) { continue; }
				MPI_Intercomm_create(pgroup, 0, MPI_COMM_WORLD, ranks[(gid+1)*merge_size], 0, &inter_pgroup);
				MPI_Intercomm_merge(inter_pgroup, 0 /* LOW */, &pgroup);

			} else {

				MPI_Intercomm_create(pgroup, 0, MPI_COMM_WORLD, ranks[(gid-1)*merge_size], 0, &inter_pgroup);
				MPI_Intercomm_merge(inter_pgroup, 1 /* HIGH */, &pgroup);

			}

			MPI_Comm_free(&inter_pgroup);

			if (pgroup_old != MPI_COMM_SELF) { MPI_Comm_free(&pgroup_old); }
		}

		return pgroup;
	}

	void call_back(int sig) { LOG(DEBUG) << "Time to wakeup"; }

} // end anonymous namespace 


namespace mpits {

	ctx::fcontext_t fcw;
	ctx::fcontext_t* curr_ptr=nullptr;

	// Send the request to the Scheduler, wait for the TaskID and return 
	Task::TaskID Worker::spawn(const std::string& kernel, unsigned min, unsigned max) {

		using namespace comm;

		auto task_data = std::make_tuple(kernel, min, max);
		SendChannel()( Message(Message::TASK_CREATE, 0, node_comm(), task_data) );
		
		Task::TaskID tid;
		MPI_Recv(&tid, 1, MPI_UNSIGNED_LONG, 0, 0, node_comm(), MPI_STATUS_IGNORE);
		
		LOG(DEBUG) << "Task generated: " << tid;

		return tid;
	}

	void Worker::finalize() {

		assert(curr_active_task != active_tasks.end() && "curr task pointer is not valid!");

		TaskDesc& desc = *curr_active_task->second;

		// Makes sure that all the workers have reached the end of the kernel 
		MPI_Barrier(desc.comm());

		int rank;
		MPI_Comm_rank(desc.comm(), &rank);

		// kernel completition
		if (rank==0) {
			LOG(DEBUG) << "Kernel completed!";
			// Send the completition message to the scheduler 
			comm::SendChannel()( comm::Message(comm::Message::TASK_COMPLETED, 0, node_comm(), std::make_tuple(desc.tid())) );
		}

		assert(curr_ptr && "Curr context pointer is invalid, how did you manage to jump here?");
		auto* ptr = curr_ptr;
		curr_ptr = &fcw;
		
		// erase task from the map 
		ctx_clean.emplace_back( std::move(curr_active_task->second) );

		active_tasks.erase(curr_active_task);
		curr_active_task = active_tasks.end();

		ctx::jump_fcontext(ptr, &fcw, 0);
	}

	void Worker::do_work() {

		signal(SIGCONT, call_back);
		LOG(INFO) << "Starting worker";
		
		// Initialize the context 
		ctx::guarded_stack_allocator alloc;
        std::size_t size = ctx::guarded_stack_allocator::minimum_stacksize();
        
		bool stop=false;

		LOG(DEBUG) << "Opening kernel.so...";
		void* handle = dlopen("./libkernels.so", RTLD_NOW);
		if (!handle) {
			LOG(ERROR) << "Cannot open library: " << dlerror();
			::exit(1);
		}

		while (!stop) {
			LOG(DEBUG) << "Sleep";

			pause();
			
			MPI_Status status;
			MPI_Probe(0, MPI_ANY_TAG, node_comm(), &status);

			switch (status.MPI_TAG) {

			case 0: // Exit 
			{
				stop = true;
				break;
			}

			case 1:  // Spawn task 
			{
				int size;
				// we expect to receive a list of integers representing 
				// the process ranks which will form the group 
				MPI_Get_count(&status, MPI_INT, &size);

				std::vector<int> ranks(size);
				MPI_Recv(&ranks.front(), size, MPI_INT, 0, 
						 status.MPI_TAG, node_comm(), MPI_STATUS_IGNORE);

				LOG(DEBUG) << "MAKING GROUP " << utils::join(ranks);

				// Create group
				MPI_Comm comm = make_group(*this,ranks);
				MPI_Barrier(comm);

				char* kernel_name = nullptr;

				int new_rank;
				MPI_Comm_rank(comm, &new_rank);

				unsigned long tid;

				if (new_rank==0) {
					LOG(DEBUG) << "GROUP FORMED!";

					// retrieve tid
					MPI_Recv(&tid, 1, MPI_UNSIGNED_LONG, 0, 0, node_comm(), MPI_STATUS_IGNORE);

					MPI_Bcast(&tid, 1, MPI_UNSIGNED_LONG, 0, comm);
					// retrieve work 
					MPI_Status status;
					MPI_Probe(0, MPI_ANY_TAG, node_comm(), &status); 

					assert(status.MPI_TAG == 0);
					MPI_Get_count(&status, MPI_CHAR, &size);
					kernel_name = new char[size];

					MPI_Recv(kernel_name, size, MPI_CHAR, 0, 0, node_comm(), MPI_STATUS_IGNORE);

					MPI_Bcast(&size, 1, MPI_INT, 0, comm);
					MPI_Bcast(kernel_name, size, MPI_CHAR, 0, comm);
				} else {
					// remaining processes in the group retirive:guarded_stack_allocator
					//  	tid
					MPI_Bcast(&tid, 1, MPI_UNSIGNED_LONG, 0, comm);
					//		kernel_name_size
					MPI_Bcast(&size, 1, MPI_INT, 0, comm);
					kernel_name = new char[size];
					//		kernel_name 
					MPI_Bcast(kernel_name, size, MPI_CHAR, 0, comm);
				}

				LOG(DEBUG) << "TID: " << tid << " - recvd kernel '" << kernel_name << "'";

				typedef void (*kernel_t)(intptr_t);

				// reset errors
				dlerror();
				kernel_t kernel = (kernel_t) dlsym(handle, kernel_name);

				const char *dlsym_error = dlerror();
				if (dlsym_error) {
					LOG(ERROR) << "Cannot load symbol '" << kernel_name << "': " << dlsym_error;
					dlclose(handle);
				}
		
				// use it to do the calculation
				LOG(DEBUG) << "Calling '" << kernel_name << "'...";
				delete[] kernel_name;
			
				auto* stack = alloc.allocate(size);
				auto* fc = ctx::make_fcontext( stack, size, kernel );
				
				curr_active_task = active_tasks.insert( 
					std::make_pair(
						tid,  
						std::unique_ptr<TaskDesc>( new TaskDesc(tid, comm, fc, stack, alloc) )
					)).first;

				curr_ptr = fc;
				ctx::jump_fcontext( &fcw, curr_ptr, (intptr_t)&comm);
				break;
			}

			case 3:	// Resume Worker 
			{
				Task::TaskID tid;
				MPI_Recv(&tid, 1, MPI_UNSIGNED_LONG, 0, 3, node_comm(), MPI_STATUS_IGNORE);
				
				auto fit = active_tasks.find( tid );
				assert(fit != active_tasks.end() && "Scheduler required to resume completed task");

				curr_ptr = fit->second->ctx();
				ctx::jump_fcontext( &fcw, curr_ptr, 0);
				break;
			}

			default:
				assert(false);
			}

			ctx_clean.clear();
		}

		LOG(INFO) << "\{W@} Worker Exiting!";
		dlclose(handle);
		MPI_Finalize();

	}

	void Worker::wait_for(const Task::TaskID& tid) {

		assert(curr_active_task != active_tasks.end() && "curr task pointer is not valid!");

		TaskDesc& desc = *curr_active_task->second;

		/**
		 * If the task id is the current one then we don't need to wait
		 */
		if (tid == desc.tid()) { return; }

		int rank;
		MPI_Comm_rank(desc.comm(), &rank);

		if (rank==0) {
			// Let the scheduler know that this task is now suspended waiting for tid 
			comm::SendChannel()( comm::Message(comm::Message::TASK_WAIT, 0, node_comm(), 
								 std::make_tuple(desc.tid(), std::vector<unsigned long>({tid}))) 
					);
		}

		auto* ptr = curr_ptr;
		curr_ptr = &fcw;
		
		// erase task from the map 
		curr_active_task = active_tasks.end();

		ctx::jump_fcontext(ptr, &fcw, 0);
	}

} // end namespace mpits 
