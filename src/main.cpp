
#include <iostream>

#include "comm/init.h"

#include "utils/logging.h"
#include "utils/string.h"

using namespace std;
using namespace mpits::utils;

int main(int argc, char* argv[]) {

	mpits::init();

	LOG(INFO) << "MPI Task System";

	std::vector<int> vec{ 1, 2, 3, 4, 5 };
	LOG(INFO) << join(vec.begin(), vec.end());
	LOG(INFO) << join(vec.begin(), vec.end(), "@", 
						[](const int& cur) { return cur*cur; }
				);

	LOG(INFO) << join(vec, ",", [](const int& cur) { return cur*cur; });

	LOG(INFO) << join(std::vector<std::string>({ "1", "2", "3", "$" }), "," );

	LOG(INFO) << join({ 1, 2 }, "####", [&](const int& cur) { return cur*cur; } );

	mpits::finalize();
}
