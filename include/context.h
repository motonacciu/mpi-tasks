
#pragma once 

#include <unistd.h>
#include <csignal>

namespace mpits {

struct Role {
	
	enum RoleType { RT_SCHEDULER, RT_WORKER };

	Role(const RoleType& type) : m_type(type) { }

	const RoleType& type() const { return m_type; }

	virtual void do_work() = 0;

	virtual ~Role() { }

private:
	RoleType m_type;
};

} // end mpits namespace 
