
#pragma once 

#include "context.h"
#include <memory>

namespace mpits {

	void init();

	void finalize();

	std::unique_ptr<Role> assign_roles(unsigned nprocs);

} // end mpits namespace 
