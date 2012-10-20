#include <gtest/gtest.h>
#include "comm/message.h"
#include "utils/string.h"

#include <sstream>
#include <vector>

#include <mpi.h>

using namespace mpits::comm;
using namespace mpits::utils;

TEST(Message, CreationVecInt) {

	Message m(Message::GROUP_CREATE, 1, MPI_COMM_WORLD, std::vector<int>({10,20,30}));
	EXPECT_EQ(Message::GROUP_CREATE, m.msg_id());
	EXPECT_EQ(40u, m.size());
	EXPECT_EQ(std::vector<int>({10,20,30}), m.get_content_as<std::vector<int>>());
	
	std::ostringstream ss;
	ss << join(m.get_content_as<std::vector<int>>());
	EXPECT_EQ("10,20,30", ss.str());
	
}

TEST(Message, CreationString) {

	Message m(Message::GROUP_CREATE, 1, MPI_COMM_WORLD, std::string("host1"));
	EXPECT_EQ(Message::GROUP_CREATE, m.msg_id());
	EXPECT_EQ(35u, m.size());
	EXPECT_EQ("host1", m.get_content_as<std::string>());

}
