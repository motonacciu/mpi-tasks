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
		std::function<bool (const size_t&)>(
			[](const size_t& val) { std::cout << val << std::endl; return false; })
	);

	auto handler = std::thread(std::ref(eh));

	eh.queue().push( Event(Event::TASK_CREATE,10ul) );
	eh.queue().push( Event(Event::TASK_CREATE,11ul) );
	sleep(1);
	eh.queue().push( Event(Event::TASK_CREATE,12ul) );

	eh.queue().push( Event(Event::SHUTDOWN,true) );
	sleep(1);

	handler.join();

}

TEST(Events, TimedEvents) {

	Logger::get(std::cout, DEBUG);

	EventHandler eh;
	eh.connect(Event::TASK_CREATE, 
		std::function<bool (const size_t&)>(
			[](const size_t& val) { std::cout << val << std::endl; return false; })
	);

	auto handler = std::thread(std::ref(eh));

	eh.queue().push( 
			Event(Event::TASK_CREATE, 10ul,  
				  std::chrono::system_clock::now() + std::chrono::seconds(2)
			) 
		);
	eh.queue().push( Event(Event::TASK_CREATE,11ul) );
	eh.queue().push( Event(Event::TASK_CREATE,12ul) );

	sleep(5);
	eh.queue().push( Event(Event::SHUTDOWN,true) );

	handler.join();

	MPI_Finalize();
}
