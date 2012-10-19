
#include <iostream>

#include "comm/init.h"

#include "utils/logging.h"
#include "utils/string.h"

using namespace std;
using namespace mpits::utils;

int main(int argc, char* argv[]) {

	mpits::init();

	LOG(INFO) << "MPI Task System";



	mpits::finalize();
}
