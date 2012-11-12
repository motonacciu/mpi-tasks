
#pragma once 

#include <unistd.h>
#include <csignal>

#include "task.h"

namespace mpits {

struct Role {
	
	enum RoleType { RT_SCHEDULER, RT_WORKER };

	Role(const RoleType& type, const MPI_Comm& node_comm) : 
		m_type(type), m_node_comm(node_comm) 
	{ 
		MPI_Comm_size(node_comm, &m_node_size);
		MPI_Comm_rank(node_comm, &m_node_rank);
	}

	const RoleType& type() const { return m_type; }

	const MPI_Comm& node_comm() const { return m_node_comm; }

	int node_rank() const { return m_node_rank; }

	int node_size() const { return m_node_size; }

	virtual void do_work() = 0;

	virtual Task::TaskID spawn(const std::string& kernel, unsigned min, unsigned max) = 0;

	virtual void wait_for(const Task::TaskID& tid) = 0;

	virtual Task::TaskID get_tid() { } 

	virtual void finalize() = 0;

	virtual ~Role() { }

private:
	RoleType m_type;

	MPI_Comm m_node_comm;
	int 	 m_node_size;
	int		 m_node_rank;
};

} // end mpits namespace 
