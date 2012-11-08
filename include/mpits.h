#pragma once 

#include "task.h"
#include "utils/logging.h"

namespace mpits {

void init(std::ostream& log_stream=std::cerr, const Level& level=DEBUG);

Task::TaskID spawn(const std::string& kernel, unsigned min, unsigned max);

void wait_for(const Task::TaskID& tid);

void finalize();
		
void exit();

} // end namespace mpits 
