#pragma once 

#include "task.h"

namespace mpits {

void init();

Task::TaskID spawn(const std::string& kernel, unsigned min, unsigned max);


void finalize();
		
} // end namespace mpits 
