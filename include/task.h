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
		return; 
	}

	// this code is executed by the scheduler 
	if (rank != 0) {
		EventHandler handler;
		handler();
		return;
	}


}

void make_group(const std::vector<int>& ranks) {
		
	// find pids 
	auto& r = static_cast<Scheduler&>(get_role());
	
	for(int idx : ranks) {
		kill(r.pid_list()[idx-1].second, SIGINT);
		MPI_Send(const_cast<int*>(&ranks.front()), ranks.size(), 
				 MPI_INT, r.pid_list()[idx-1].first, 1, r.node_comm()
		);
	}
	
	MPI_Barrier(r.node_comm());

	

}


} // end namespace mpits 
