
#include <gtest/gtest.h>
#include "utils/queue.h"

#include <sstream>
#include <vector>

#include <future>

using namespace mpits::utils;

TEST(Queue, ProducerConsumer) {

	BlockingQueue<int> bq;

	bq.push(2);
	bq.push(3);

	EXPECT_EQ(2u, bq.size());

	EXPECT_EQ(2u, bq.pop());
	EXPECT_EQ(1u, bq.size());

	EXPECT_EQ(3u, bq.pop());
	EXPECT_EQ(0, bq.size());

}

TEST(Queue, ProducerConsumer2) {

	BlockingQueue<std::string, std::vector> bq;

	auto f1 = std::async([&](){ 
			bq.push("hello"); 
			/* sleep 1 sec */
			std::this_thread::sleep_for(std::chrono::seconds(1));

			bq.push(" "); 
			bq.push("world!"); 
		});

	auto f2 = std::async([&](){ 
			EXPECT_EQ("hello", bq.pop());
			EXPECT_EQ(" ", bq.pop());
			EXPECT_EQ("world!", bq.pop());
		});

	f1.wait();
	f2.wait();

}

class obj {
public:
	obj() {
		std::cout << "Calling constructor" << std::endl;
	}

	obj(const obj&) {
		std::cout << "Calling copy constructor" << std::endl;
	}

	obj(obj&&) {
		std::cout << "Calling copy move constructor" << std::endl;
	}

	obj& operator=(const obj&) {
		std::cout << "Calling assignment op" << std::endl;
		return *this;
	}

	obj& operator=(obj&&) {
		std::cout << "Calling assignment move op" << std::endl;
		return *this;
	}

	~obj() { 
		std::cout << "Calling destructor" << std::endl;
	}
};


TEST(Queue, CustomObj) {

	BlockingQueue<obj> bq;

	bq.push( obj() );
	bq.push( obj() );

	obj o1 = bq.pop();
	obj o2 = bq.pop();

}


