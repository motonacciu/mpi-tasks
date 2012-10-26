
#pragma once 

#include <mpi.h>

#include "context.h"

namespace mpits {

struct Worker : public Role {

	Worker(const MPI_Comm& node_comm) : 
		Role(Role::RT_WORKER), 
		m_pid(getpid()),
		m_node_comm(node_comm) { }

	const pid_t& pid() const { return m_pid; }

	const MPI_Comm& node_comm() const { return m_node_comm; }

	int node_rank() const {
		int node_rank;
		MPI_Comm_rank(m_node_comm, &node_rank);
		return node_rank;
	}

	void do_work();

private:
	pid_t 		m_pid;
	MPI_Comm	m_node_comm;
};

} // end namespace mpits 
