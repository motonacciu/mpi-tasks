#pragma once 

#include "task.h"

namespace mpits {

void init();

Task::TaskID spawn(const std::string& kernel, unsigned min, unsigned max);

void wait_for(const Task::TaskID& tid);

void finalize();
		
} // end namespace mpits 
