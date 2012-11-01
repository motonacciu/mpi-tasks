#pragma once 

#include <mpi.h>

#include <memory>
#include <vector>

namespace mpits {

struct Task {

	typedef unsigned long TaskID;

	const TaskID& taskID() const { return m_tid; }

	Task(const TaskID& tid, const std::string& kernel, unsigned min, unsigned max) 
		: m_tid(tid), m_kernel(kernel), m_min(min), m_max(max) { }

	Task::TaskID tid() const { return m_tid; }

	const std::string& kernel() const { return m_kernel; }

	unsigned min() const { return m_min; }

	unsigned max() const { return m_max; }

private:

	TaskID 			m_tid;
	std::string 	m_kernel;
	unsigned 		m_min;
	unsigned 		m_max;
};


struct RemoteTask: public Task {


};

//struct LocalTask: public Task {
//	
//	typedef std::vector<int> RankList;
//
//	LocalTask(const Task::TaskID& tid, const RankList& pids) :
//		Task(tid), m_pids(pids) { }
//
//	const RankList& pids() const { return m_pids; }
//
//private:
//
//	RankList m_pids;
//
//};

} // end namespace mpits 

namespace std {

	inline std::ostream& operator<<(std::ostream& out, const mpits::Task& cur) {
		return out << "[TID:" << cur.tid() << "]";
	}

} // end std namespace 
