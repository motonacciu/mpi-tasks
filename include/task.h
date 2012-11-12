#pragma once 

#include <mpi.h>

#include <memory>
#include <vector>

namespace mpits {


struct Task {
	
	enum Status { TS_READY, TS_RUN, TS_WAIT };

	typedef unsigned long TaskID;


	const TaskID& taskID() const { return m_tid; }

	Task(const TaskID& tid, const std::string& kernel, unsigned min, unsigned max) 
		: m_tid(tid), 
		  m_kernel(kernel), 
		  m_min(min), 
		  m_max(max) { }

	const Task::TaskID& tid() const { return m_tid; }

	const std::string& kernel() const { return m_kernel; }

	unsigned min() const { return m_min; }

	unsigned max() const { return m_max; }

private:

	TaskID 			m_tid;
	std::string 	m_kernel;

	unsigned 		m_min;
	unsigned 		m_max;
};

typedef std::shared_ptr<Task> TaskPtr;


struct RemoteTask: public Task {


};


struct LocalTask: public Task {
	
	typedef std::vector<int> RankList;

	LocalTask(const Task& tid, const RankList& ranks) :
		Task(tid), m_ranks(ranks) { }

	const RankList& ranks() const { return m_ranks; }

	Task::Status& status() { return m_ts; }
	const Task::Status& status() const { return m_ts; }

private:

	RankList m_ranks;
	Task::Status m_ts;

};

typedef std::shared_ptr<LocalTask> LocalTaskPtr;

} // end namespace mpits 

namespace std {

	inline std::ostream& operator<<(std::ostream& out, const mpits::Task& cur) {
		return out << "[TID:" << cur.tid() << "]";
	}

} // end std namespace 
