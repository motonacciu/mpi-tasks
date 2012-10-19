
#include <gtest/gtest.h>
#include "utils/string.h"

#include <unistd.h>
#include <csignal>
#include <sstream>
#include <vector>

#include <chrono>

using namespace mpits::utils;

void call_back(int sig) {
	auto t = std::chrono::high_resolution_clock::now();
	std::cout << "awake " << t.time_since_epoch().count() << std::endl;
}

/**
 * Tests the overhead due to the wake up of a sleeping process 
 */
TEST(Signals, PauseWakeup) {

	pid_t childpid;
	pid_t father=getpid();
	
	signal(SIGINT, call_back);

	/* now create new process */
    childpid = fork();
	
	if (childpid >= 0) { /* fork succeeded */ 

        if (childpid == 0) { /* fork() returns 0 to the child process */
			sleep(2);

			// get the time
			auto t = std::chrono::high_resolution_clock::now();
			std::cout << "wakeup " << t.time_since_epoch().count() << std::endl;
			kill (father, SIGINT);
			exit(0);
		} else {
			pause();
		}
	}

}


