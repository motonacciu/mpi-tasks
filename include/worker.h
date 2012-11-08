
#pragma once 

#include <mpi.h>

#include "context.h"

namespace mpits {

struct Worker : public Role {

	Worker(const MPI_Comm& node_comm) : 
		Role(Role::RT_WORKER, node_comm), 
		m_pid(getpid()) { }

	const pid_t& pid() const { return m_pid; }

	void do_work();

	Task::TaskID spawn(const std::string& kernel, unsigned min, unsigned max);

	void wait_for(const Task::TaskID& tid) {} 

	void finalize();

private:

	pid_t 	m_pid;

};

} // end namespace mpits 
