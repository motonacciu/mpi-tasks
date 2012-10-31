
#pragma once 

#include <mpi.h>

#include "context.h"
#include "event.h"

namespace mpits {

struct Scheduler : public Role {

	typedef std::pair<int, int> 	PidPair;
	typedef std::vector<PidPair> 	Pids;

	Scheduler(const MPI_Comm& 	node_comm, 
			  const MPI_Comm& 	sched_comm,
			  Pids&&	 		pids) 
	: 
		Role(Role::RT_SCHEDULER), 
		m_node_comm(node_comm),
		m_sched_comm(sched_comm),
		m_pids(std::move(pids)),
		m_thr(std::ref(m_handler)) { }

	const MPI_Comm& node_comm() const { return m_node_comm; }

	int node_rank() const {
		int node_rank;
		MPI_Comm_rank(m_node_comm, &node_rank);
		return node_rank;
	}

	int sched_rank() const {
		int sched_rank;
		MPI_Comm_rank(m_sched_comm, &sched_rank);
		return sched_rank;
	}

	const Pids& pid_list() const { return m_pids; }

	void do_work() { }
		
	Scheduler& operator=(const Scheduler&) = delete;
	Scheduler(const Scheduler&) = delete;

private:
	MPI_Comm		m_node_comm;
	MPI_Comm		m_sched_comm;
	Pids			m_pids;

	EventHandler 	m_handler;
	std::thread     m_thr;
};

} // end namespace mpits 
