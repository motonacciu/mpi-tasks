
#pragma once 

#include "context.h"
#include <memory>

namespace mpits {

	std::unique_ptr<Role> assign_roles(unsigned nprocs);

} // end mpits namespace 
