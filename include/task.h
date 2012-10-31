#pragma once 

#include <mpi.h>

#include <memory>
#include <vector>

#include "context.h"

namespace mpits {


Role& get_role(std::unique_ptr<Role>&& role = std::unique_ptr<Role>());


void init();


void make_group(const std::vector<int>& ranks);


void finalize();
		
} // end namespace mpits 
