
#include <iostream>

#include "mpits.h"

#include "utils/logging.h"
#include "utils/string.h"

using namespace std;
using namespace mpits::utils;

int main(int argc, char* argv[]) {

	mpits::init();

	LOG(INFO) << "MPI Task System";

	mpits::spawn("kernel_1", 2, 4);

	sleep(5);

	mpits::finalize();
}
