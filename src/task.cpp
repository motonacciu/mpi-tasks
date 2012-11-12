
#include "mpits.h"
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

	void init(std::ostream& log_stream, const Level& level) { 

		MPI_Init(NULL, NULL);
		
		/* Initialize the logger */
		Logger::get(log_stream, level);

		int nprocs, rank;
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

		auto& role = get_role( assign_roles(nprocs) );
		role.do_work();
		
		if (role.type() == Role::RT_WORKER) {
			::exit(0);
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


	Task::TaskID get_tid() {
			
		auto& r = get_role();
		r.get_tid();
		
	}

	void finalize() {
			
		auto& r = get_role();
		r.finalize();
		
	}

} // end mpits namespace 
