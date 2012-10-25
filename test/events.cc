#include <gtest/gtest.h>

#include "event.h"

#include <sstream>
#include <vector>

#include <thread>

using namespace mpits;
using namespace mpits::utils;

TEST(Events, Test1) {

	MPI_Init(NULL,NULL);
	Logger::get(std::cout, DEBUG);

	EventHandler eh;
	eh.connect(Event::TASK_CREATE, 
		std::function<bool (int const&)>(
			[](const int& val) { std::cout << val << std::endl; return false; })
	);

	auto handler = std::thread(std::ref(eh));

	eh.queue().push( Event(Event::TASK_CREATE,10) );
	eh.queue().push( Event(Event::TASK_CREATE,11) );
	sleep(1);
	eh.queue().push( Event(Event::TASK_CREATE,12) );

	eh.queue().push( Event(Event::SHUTDOWN,true) );
	sleep(1);

	handler.join();

}

TEST(Events, TimedEvents) {

	Logger::get(std::cout, DEBUG);

	EventHandler eh;
	eh.connect(Event::TASK_CREATE, 
		std::function<bool (int const&)>(
			[](const int& val) { std::cout << val << std::endl; return false; })
	);

	auto handler = std::thread(std::ref(eh));

	eh.queue().push( 
			Event(Event::TASK_CREATE, 10,  
				  std::chrono::system_clock::now() + std::chrono::seconds(2)
			) 
		);
	eh.queue().push( Event(Event::TASK_CREATE,11) );
	eh.queue().push( Event(Event::TASK_CREATE,12) );

	sleep(5);
	eh.queue().push( Event(Event::SHUTDOWN,true) );

	handler.join();

	MPI_Finalize();
}
