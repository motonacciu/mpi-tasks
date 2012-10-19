
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


