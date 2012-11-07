
#include "worker.h"

#include "comm/message.h"
#include "comm/channel.h"

#include "utils/logging.h"
#include "utils/string.h"

#include <dlfcn.h>

namespace {
//
// taken the idea from:
// 	https://svn.mcs.anl.gov/repos/mpi/mpich2/trunk/test/mpi/spawn/pgroup_intercomm_test.c
//
MPI_Comm make_group(const mpits::Worker& worker, const std::vector<int>& ranks) {
	
	/* CASE: Group size 0 */
	assert (!ranks.empty() );

	/* CASE: Group size 1 */
	if (ranks.size() == 1 && ranks.front() == worker.node_rank()) {
		return MPI_COMM_SELF;
	}

	auto fit = std::find(ranks.begin(), ranks.end(), worker.node_rank());
	assert(fit != ranks.end());

	size_t idx = std::distance(ranks.begin(), fit);

	MPI_Comm pgroup = MPI_COMM_SELF;

	for(unsigned merge_size = 1; merge_size < ranks.size(); merge_size *= 2) {
	   
		unsigned gid = idx / merge_size;
 		MPI_Comm pgroup_old = pgroup, inter_pgroup;

		if (gid % 2 == 0) {
			/* Check if right partner doesn't exist */
			if ((gid+1)*merge_size >= ranks.size()) { continue; }

			MPI_Intercomm_create(pgroup, 0, MPI_COMM_WORLD, ranks[(gid+1)*merge_size], 
								 0, &inter_pgroup);

			MPI_Intercomm_merge(inter_pgroup, 0 /* LOW */, &pgroup);
		} else {
			MPI_Intercomm_create(pgroup, 0, MPI_COMM_WORLD, ranks[(gid-1)*merge_size], 
								 0, &inter_pgroup);

			MPI_Intercomm_merge(inter_pgroup, 1 /* HIGH */, &pgroup);
		}

		MPI_Comm_free(&inter_pgroup);

		if (pgroup_old != MPI_COMM_SELF) { MPI_Comm_free(&pgroup_old); }
	}

	return pgroup;
}

void call_back(int sig) {
	LOG(INFO) << "Time to wakeup";
}

} // end anonymous namespace 

namespace mpits {

// Send the request to the Scheduler, wait for the TaskID and return 
Task::TaskID Worker::spawn(const std::string& kernel, unsigned min, unsigned max) {

	using namespace comm;

	auto task_data = std::make_tuple(kernel, min, max);
	//msg_content_traits<decltype(task_data)>::to_bytes()
	SendChannel()( Message(Message::TASK_CREATE, 0, node_comm(), task_data) );
	
	Task::TaskID tid;
	MPI_Recv(&tid, 1, MPI_UNSIGNED_LONG, 0, 0, node_comm(), MPI_STATUS_IGNORE);
	
	LOG(INFO) << "Task generated: " << tid;
	return tid;
}


void Worker::do_work() {

	signal(SIGCONT, call_back);
	LOG(INFO) << "Starting worker";

	bool stop=false;

	while (!stop) {
		LOG(INFO) << "Sleep";

		pause();
		
		MPI_Status status;
		MPI_Probe(0, MPI_ANY_TAG, node_comm(), &status);

		switch (status.MPI_TAG) {

		case 0:
		{
			stop = true;
			break;
		}


		case 1: 
		{
			int size;
			// we expect to receive a list of integers representing 
			// the process ranks which will form the group 
			MPI_Get_count(&status, MPI_INT, &size);

			std::vector<int> ranks(size);
			MPI_Recv(&ranks.front(), size, MPI_INT, 0, 
					 status.MPI_TAG, node_comm(), MPI_STATUS_IGNORE);

			LOG(INFO) << "MAKING GROUP " << utils::join(ranks);

			// Create group
			MPI_Comm comm = make_group(*this,ranks);
			MPI_Barrier(comm);

			char* kernel_name = nullptr;

			int new_rank;
			MPI_Comm_rank(comm, &new_rank);

			unsigned long tid;

			if (new_rank==0) {
				LOG(INFO) << "GROUP FORMED!";

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
				// remaining processes in the group retirive:
				//  	tid
				MPI_Bcast(&tid, 1, MPI_UNSIGNED_LONG, 0, comm);
				//		kernel_name_size
				MPI_Bcast(&size, 1, MPI_INT, 0, comm);
				kernel_name = new char[size];
				//		kernel_name 
				MPI_Bcast(kernel_name, size, MPI_CHAR, 0, comm);
			}

			LOG(INFO) << "TID: " << tid << " - recvd kernel '" << kernel_name << "'";
			// Do-work TODO
			//
			
			LOG(INFO) << "Opening kernel.so...";
 			void* handle = dlopen("./libkernels.so", RTLD_LAZY);
			if (!handle) {
		        LOG(ERROR) << "Cannot open library: " << dlerror();
				exit(1);
		    }

			typedef void (*kernel_t)(MPI_Comm);

 			// reset errors
		 	dlerror();
			kernel_t kernel = (kernel_t) dlsym(handle, kernel_name);

 			const char *dlsym_error = dlerror();
			if (dlsym_error) {
    			LOG(ERROR) << "Cannot load symbol '" << kernel_name << "': " << dlsym_error;

 	    	   dlclose(handle);
    		}
    
 			// use it to do the calculation
			LOG(INFO) << "Calling '" << kernel_name << "'...";
			kernel(comm);

			// Kernel completed
			MPI_Comm_free(&comm);
			delete[] kernel_name;

			if (new_rank==0) {
				LOG(INFO) << "Kernel completed!";
				// Send the completition message to the scheduler 
				comm::SendChannel()( comm::Message(comm::Message::TASK_COMPLETED, 0, node_comm(), std::make_tuple(tid)) );
			}
			break;
		}

		default:
			assert(false);
		}

	}

	LOG(INFO) << "Worker end";

	MPI_Finalize();

}

} // end namespace mpits 
