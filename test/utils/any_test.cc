
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
	any b(s);
	EXPECT_EQ(s, b.as<std::string>());
}
