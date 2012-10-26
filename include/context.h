
#pragma once 

#include <unistd.h>
#include <csignal>

namespace mpits {

struct Role {
	
	enum RoleType { RT_SCHEDULER, RT_WORKER };

	Role(const RoleType& type) : m_type(type) { }

	const RoleType& type() const { return m_type; }

	virtual ~Role() { }

private:
	RoleType m_type;
};


class Context {

public:


};

} // end mpits namespace 
