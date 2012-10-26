
#include "worker.h"

#include "utils/logging.h"

namespace mpits {
//
// taken the idea from:
// 	https://svn.mcs.anl.gov/repos/mpi/mpich2/trunk/test/mpi/spawn/pgroup_intercomm_test.c
//
MPI_Comm make_group(const Worker& worker, const std::vector<int>& ranks) {
	
	/* CASE: Group size 0 */
	assert (!ranks.empty() );

	/* CASE: Group size 1 */
	if (ranks.size() == 1 && ranks.front() == worker.node_rank()) {
		return MPI_COMM_SELF;
	}

	auto fit = std::find(ranks.begin(), ranks.end(), worker.node_rank());
	assert(fit != ranks.end());

	size_t idx = std::distance(ranks.begin(), fit);

	MPI_Comm pgroup = MPI_COMM_SELF;

	for(int merge_size = 1; merge_size < ranks.size(); merge_size *= 2) {
	   
		int gid = idx / merge_size;

 		MPI_Comm pgroup_old = pgroup, inter_pgroup;

		if (gid % 2 == 0) {
			/* Check if right partner doesn't exist */
			if ((gid+1)*merge_size >= ranks.size()) { continue; }

			MPI_Intercomm_create(pgroup, 0, MPI_COMM_WORLD, ranks[(gid+1)*merge_size], 0, &inter_pgroup);
			MPI_Intercomm_merge(inter_pgroup, 0 /* LOW */, &pgroup);
		} else {
			MPI_Intercomm_create(pgroup, 0, MPI_COMM_WORLD, ranks[(gid-1)*merge_size], 0, &inter_pgroup);
			MPI_Intercomm_merge(inter_pgroup, 1 /* HIGH */, &pgroup);
		}

		MPI_Comm_free(&inter_pgroup);

		if (pgroup_old != MPI_COMM_SELF) { MPI_Comm_free(&pgroup_old); }
	}

	return pgroup;
}


void Worker::do_work() {

	LOG(INFO) << "Starting worker";

//	pause();
//
//	int cmd;
//	MPI_Recv(&cmd, 1, MPI_INT, 0, 0, node_comm(), MPI_STATUS_IGNORE);
//
//	if (cmd == 0) {
//		// Create group
		MPI_Comm comm = make_group(*this, {1,2,3});
		int m;
		if ( node_rank()==1 ) { m=10; }	
		MPI_Bcast(&m, 1, MPI_INT, 0, comm);

		LOG(INFO) << m;
//	}

	LOG(INFO) << "Worker end";
}

} // end namespace mpits 
