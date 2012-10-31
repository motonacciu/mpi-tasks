
#include <iostream>

#include "task.h"

#include "utils/logging.h"
#include "utils/string.h"

using namespace std;
using namespace mpits::utils;

int main(int argc, char* argv[]) {

	mpits::init();

	LOG(INFO) << "MPI Task System";

	mpits::make_group({2,4,6});

	sleep(5);

	mpits::finalize();
}
