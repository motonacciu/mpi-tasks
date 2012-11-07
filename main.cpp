
#include <iostream>

#include "mpits.h"

#include "utils/logging.h"
#include "utils/string.h"

using namespace std;
using namespace mpits::utils;

int main(int argc, char* argv[]) {

	mpits::init();

	LOG(INFO) << "MPI Task System";

	auto tid = mpits::spawn("kernel_1", 2, 4);

	mpits::wait_for(tid);

	mpits::finalize();
}
