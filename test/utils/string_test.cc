
#include <gtest/gtest.h>
#include "utils/string.h"

#include <sstream>
#include <vector>

using namespace mpits::utils;

TEST(StringUtils, JoinLValuesIter) {

	std::vector<int> vec{ 1, 2, 3, 4, 5 };
	std::ostringstream ss;
	ss << join(vec.begin(), vec.end());

	EXPECT_EQ("1,2,3,4,5", ss.str());

	ss.str("");
	ss << join(vec.begin(), vec.end(), ",", [](const int& cur) { return cur*cur; });
	EXPECT_EQ("1,4,9,16,25", ss.str());
}

TEST(StringUtils, JoinLValueCont) {

	std::vector<int> vec{ 1, 2, 3, 4, 5 };
	std::ostringstream ss;

	ss << join(vec, ":", [](const int& cur) { return cur+1; });
	EXPECT_EQ("2:3:4:5:6", ss.str());

}

TEST(StringUtils, JoinRValueCont) {

	std::vector<int> vec{ 1, 2, 3, 4, 5 };
	std::ostringstream ss;                	
	
	ss << join(std::vector<std::string>({ "1", "2", "3", "$" }), "," );
	EXPECT_EQ("1,2,3,$", ss.str());

}

TEST(StringUtils, JoinInitializerList) {

	std::vector<int> vec{ 1, 2, 3, 4, 5 };
	std::ostringstream ss;   

	ss << join({ 1, 2 }, " *** ", [&](const int& cur) { return cur*cur; } );
	EXPECT_EQ("1 *** 4", ss.str());

	ss.str("");
	int a=0, b=1, c=2;
	ss << join({&a,&b,&c}, ":", deref<int*>());
	EXPECT_EQ("0:1:2", ss.str());
}

