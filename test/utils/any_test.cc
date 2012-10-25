
#include <gtest/gtest.h>
#include "utils/any.h"

#include <sstream>
#include <vector>

using namespace mpits::utils;

TEST(Any, Simple) {

	any a(10);
	auto val = a.as<int>();
	EXPECT_EQ(10, val);
	
	std::string s("hello, world!");
	any b(std::move(s));
	EXPECT_EQ("hello, world!", b.as<std::string>());

	any c(10,20,30);
	auto t1 = std::make_tuple(10,20,30);
	auto t2 = c.as<std::tuple<int,int,int>>();
	EXPECT_EQ(t1, t2);
}
