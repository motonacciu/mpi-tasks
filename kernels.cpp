

#include <iostream>
#include <mpi.h>
#include <vector>

#include <ctime>

#include "mpits.h"

#include "utils/string.h"
#include "utils/logging.h"

extern "C" { 
	void kernel_1(intptr_t);
}

void kernel_1(intptr_t comm_ptr) {

	MPI_Comm comm = *(MPI_Comm*)comm_ptr;

	int comm_size, rank;
	MPI_Comm_size(comm, &comm_size);
	MPI_Comm_rank(comm, &rank);

	srand( rank );
	int r = rand()%100;
	
	std::vector<int> v(comm_size);

	MPI_Allgather(&r, 1, MPI_INT, &v.front(), 1, MPI_INT, comm);
	std::cout << mpits::utils::join(v, "-") << std::endl;

	for(auto& e : v) {
		e += r;
	}

	std::vector<int> ret(v.size());
	MPI_Reduce(&v.front(), &ret.front(), comm_size, MPI_INT, MPI_SUM, 0, comm);

	if (rank==0) {
		std::cout << mpits::utils::join(ret, "-") << std::endl;
	}
	
	mpits::wait_for(5);

	// mpits::Task::TaskID id = mpits::spawn("kernel_1", 2, 2);
	
	LOG(INFO) << "Task " << mpits::get_tid() << " Exiting";

	mpits::finalize();
}
