
#include <iostream>

#include "mpits.h"

#include "utils/logging.h"
#include "utils/string.h"

using namespace std;
using namespace mpits::utils;

int main(int argc, char* argv[]) {

	mpits::init(std::cout, DEBUG);

	int n = atoi(argv[1]);

	LOG(INFO) << "MPI Task System";

	for (int i=0; i<n; ++i) {
		auto tid1 = mpits::spawn("kernel_1", 2, 2);
	}


	mpits::wait_for(4);
	mpits::wait_for(1);
	mpits::wait_for(2);
	mpits::wait_for(3);

	mpits::finalize();
}
