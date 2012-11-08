
#include <iostream>

#include "mpits.h"

#include "utils/logging.h"
#include "utils/string.h"

using namespace std;
using namespace mpits::utils;

int main(int argc, char* argv[]) {

	mpits::init(std::cout, DEBUG);

	LOG(INFO) << "MPI Task System";

	for (int i=0; i<100; i++) {
		auto tid = mpits::spawn("kernel_1", i%7<=1?2:i%7, 7);
		mpits::wait_for(tid);
	}

	mpits::finalize();
}
