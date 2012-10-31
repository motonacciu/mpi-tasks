
#include "comm/init.h"

#include <set>

#include <mpi.h>
#include "utils/logging.h"
#include "utils/string.h"

#include "event.h"
#include "worker.h"
#include "scheduler.h"

#define MAX_HOSTNAME_LENGTH 25

namespace mpits {

	std::unique_ptr<Role> assign_roles(unsigned nprocs) {

		char* hostname = new char[MAX_HOSTNAME_LENGTH+1];
		char other_hostname[(MAX_HOSTNAME_LENGTH+1)*nprocs];

		// LOG(DEBUG) << "Detecting hostname for this process:";

		/* invoke the 'hostname' command and read the value */
		gethostname(hostname, MAX_HOSTNAME_LENGTH);
		
		std::string host(hostname); 
		// LOG(DEBUG) << "\tHostname: '" << hostname << "'";

		/* Collect all the hostnames from all the processes */
		MPI_Allgather( hostname, MAX_HOSTNAME_LENGTH+1, MPI_CHAR, other_hostname, 
					   MAX_HOSTNAME_LENGTH+1, MPI_CHAR, MPI_COMM_WORLD );

		/* free memory */
		delete[] hostname;

		std::set<std::string> hosts;
		for(size_t p=0; p<static_cast<size_t>(nprocs); ++p) {
			hosts.insert( std::string(&other_hostname[p*(MAX_HOSTNAME_LENGTH+1)]) );
		}
	
		// declare communicators
		MPI_Comm node_comm, sched_comm;

		auto fit = std::find(hosts.begin(), hosts.end(), host);
		assert(fit != hosts.end() && "Current host name not found!");

		MPI_Comm_split(MPI_COMM_WORLD, std::distance(hosts.begin(), fit), 0, &node_comm);

		int node_comm_size;
		int node_comm_rank;
		MPI_Comm_size(node_comm, &node_comm_size);
		MPI_Comm_rank(node_comm, &node_comm_rank);
		if (node_comm_rank==0) {
			LOG(DEBUG) << "Node communicator has " << node_comm_size << " nodes";
			LOG(DEBUG) << "Number of nodes detected is '" << hosts.size() << "'";
			LOG(DEBUG) << utils::join(hosts.begin(), hosts.end(), ",");
		}
		/* 
		 * If the rank in the new communicator is 0 then this process will act as 
		 * a node scheduler, therefore a communicator between scheduler is create
		 * to connect them 
		 */
		MPI_Comm_split(MPI_COMM_WORLD, node_comm_rank==0, 0, &sched_comm);

		LOG(DEBUG) << "\\@ Initialization completed!";

		int mypid = getpid();
		if (node_comm_rank==0) {
			std::vector<int> node_pids(node_comm_size);

			MPI_Gather(&mypid, 1, MPI_INT, 
					   &node_pids.front(), 1, MPI_INT, 
					   0, node_comm);
			
			Scheduler::Pids pids(node_comm_size);
			for(int i=1;i<node_comm_size;++i) 
				pids[i-1] = { i, node_pids[i] };

			return std::move( std::unique_ptr<Scheduler>( 
						new Scheduler(node_comm, sched_comm, std::move(pids)) ) 
					);
		}

		LOG(INFO) << mypid;
		MPI_Gather(&mypid, 1, MPI_INT, NULL, 0, MPI_INT, 0, node_comm);
		return std::move( std::unique_ptr<Worker>( new Worker(node_comm) ) );
	}

}
