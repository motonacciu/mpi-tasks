#pragma once 

#include <mpi.h>

#include "event.h"
#include "comm/init.h"
#include "utils/logging.h"

#include "scheduler.h"

namespace mpits {

Role& get_role(std::unique_ptr<Role>&& role = std::unique_ptr<Role>()) {
	static std::unique_ptr<Role> thisRole;
	if (!thisRole) { thisRole = std::move(role); }
	return *thisRole;
}

void init() { 

	MPI_Init(NULL, NULL);
	
	/* Initialize the logger */
	Logger::get(std::cout, DEBUG);

	int nprocs, rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

	auto& role = get_role( assign_roles(nprocs) );

	if (role.type() == Role::RT_WORKER) {
		role.do_work();
		exit(0);
	}

	// this code is executed by the scheduler 
	if (rank != 0) {
		EventHandler handler;
		handler();
		exit(0);
	}


}

void make_group(const std::vector<int>& ranks) {
		
	// find pids 
	auto& r = static_cast<Scheduler&>(get_role());
	
	for(int idx : ranks) {
		kill(r.pid_list()[idx-1].second, SIGCONT);
		MPI_Send(const_cast<int*>(&ranks.front()), ranks.size(), 
				 MPI_INT, r.pid_list()[idx-1].first, 1, r.node_comm()
		);
	}

	// Send the work-item (functor) to worker 0 inside the newly generated group
	MPI_Send(const_cast<char*>("kernel_1"), 12, MPI_CHAR, ranks.front(), 0, r.node_comm()); 

}

void finalize() {
		
	// find pids 
	auto& r = static_cast<Scheduler&>(get_role());
	
	for(auto& idxs : r.pid_list()) {
		kill(idxs.second, SIGCONT);
		MPI_Send(NULL, 0, MPI_BYTE, idxs.first, 0, r.node_comm());
	}

	r.cmd_queue().push( Event(Event::SHUTDOWN,true)  );
	r.join();

	MPI_Finalize();
	
}

} // end namespace mpits 
