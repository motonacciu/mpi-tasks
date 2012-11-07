
#include "task.h"

#include "event.h"
#include "scheduler.h"

#include "comm/init.h"

#include "utils/logging.h"
#include "utils/string.h"

namespace mpits {

Role& get_role(std::unique_ptr<Role>&& role=std::unique_ptr<Role>()) {
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
	role.do_work();
	
	if (role.type() == Role::RT_WORKER) {
		exit(0);
	}
}


Task::TaskID spawn(const std::string& kernel, unsigned min, unsigned max) {

	auto& r = get_role();
	return r.spawn(kernel, min, max);

}

void wait_for(const Task::TaskID& tid) {

	auto& r = get_role();
	r.wait_for(tid);

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

} // end mpits namespace 
