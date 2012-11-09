
#pragma once 

#include <mpi.h>

#include "context.h"
#include "event.h"

#include "comm/channel.h"

namespace mpits {

struct Scheduler : public Role {

	typedef std::pair<int, int> 	PidPair;
	typedef std::vector<PidPair> 	Pids;

	Scheduler(const MPI_Comm& 	node_comm,
			  const MPI_Comm& 	sched_comm,
			  Pids&&	 		pids) 
	: 
		Role(Role::RT_SCHEDULER, node_comm),
		m_tid(0),
		m_sched_comm(sched_comm),
		m_pids(std::move(pids)),
		m_rchan(m_handler),
		m_schan(m_handler),
		m_thr(std::ref(m_handler)) 
	{ 
		int n_workers;
		MPI_Comm_size(node_comm, &n_workers);
		for (int rank=1; rank<n_workers; ++rank) { m_free_ranks.insert(rank); }
	}

	int sched_rank() const {
		int sched_rank;
		MPI_Comm_rank(m_sched_comm, &sched_rank);
		return sched_rank;
	}

	size_t next_tid() { return ++m_tid; }

	const Pids& pid_list() const { return m_pids; }

	void do_work();
	 
	EventQueue& cmd_queue() { return m_handler.queue(); }

	void enqueue_task(const std::shared_ptr<Task>& task) {
		m_task_queue.push_back( task );
	}

	const std::set<int>& free_ranks() const { return m_free_ranks; }
	std::set<int>& free_ranks() { return m_free_ranks; }

	std::shared_ptr<Task> next_task() {
		assert(!m_task_queue.empty());
		for(auto it = m_task_queue.begin(), end=m_task_queue.end(); 
				it != end; ++it) 
		{
			if ((*it)->min() <= m_free_ranks.size()) {
				std::shared_ptr<Task> t = *it;
				m_task_queue.erase(it);
				return t;
			}
		}
		return std::shared_ptr<Task>();
	}

	void join() { m_thr.join(); }

	Task::TaskID spawn(const std::string& kernel, unsigned min, unsigned max);

	void wait_for(const Task::TaskID& tid);

	void finalize();

	virtual ~Scheduler() { }

	Scheduler& operator=(const Scheduler&) = delete;
	Scheduler(const Scheduler&) = delete;

private:
	size_t			m_tid;

	MPI_Comm		m_sched_comm;
	Pids			m_pids;

	EventHandler 			m_handler;
	comm::ReceiveChannel 	m_rchan;
	comm::SendChannel 		m_schan;

	std::thread     m_thr;

	std::list<std::shared_ptr<Task>> m_task_queue;

	std::set<int> m_free_ranks;
};

} // end namespace mpits 
