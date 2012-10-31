
#include "worker.h"

#include "utils/logging.h"
#include "utils/string.h"

namespace {
//
// taken the idea from:
// 	https://svn.mcs.anl.gov/repos/mpi/mpich2/trunk/test/mpi/spawn/pgroup_intercomm_test.c
//
MPI_Comm make_group(const mpits::Worker& worker, const std::vector<int>& ranks) {
	
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

	for(unsigned merge_size = 1; merge_size < ranks.size(); merge_size *= 2) {
	   
		unsigned gid = idx / merge_size;
 		MPI_Comm pgroup_old = pgroup, inter_pgroup;

		if (gid % 2 == 0) {
			/* Check if right partner doesn't exist */
			if ((gid+1)*merge_size >= ranks.size()) { continue; }

			MPI_Intercomm_create(pgroup, 0, MPI_COMM_WORLD, ranks[(gid+1)*merge_size], 
								 0, &inter_pgroup);

			MPI_Intercomm_merge(inter_pgroup, 0 /* LOW */, &pgroup);
		} else {
			MPI_Intercomm_create(pgroup, 0, MPI_COMM_WORLD, ranks[(gid-1)*merge_size], 
								 0, &inter_pgroup);

			MPI_Intercomm_merge(inter_pgroup, 1 /* HIGH */, &pgroup);
		}

		MPI_Comm_free(&inter_pgroup);

		if (pgroup_old != MPI_COMM_SELF) { MPI_Comm_free(&pgroup_old); }
	}

	return pgroup;
}

void call_back(int sig) {
	LOG(INFO) << "Time to wakeup";
}

} // end anonymous namespace 

namespace mpits {


void Worker::do_work() {

	LOG(INFO) << "Starting worker";

	bool stop=false;
	
	signal(SIGINT, call_back);

	while (!stop) {

		pause();

		MPI_Status status;
		MPI_Probe(0, MPI_ANY_TAG, node_comm(), &status);

		switch (status.MPI_TAG) {

		case 0:
			stop = true;
			break;

		case 1: 
		{
			int size;
			// we expect to receive a list of integers representing 
			// the process ranks which will form the group 
			MPI_Get_count(&status, MPI_INT, &size);

			std::vector<int> ranks(size);
			MPI_Recv(&ranks.front(), size, MPI_INT, 0, 
					 status.MPI_TAG, node_comm(), MPI_STATUS_IGNORE);

			LOG(INFO) << "MAKING GROUP " << utils::join(ranks);
			// Create group
			MPI_Comm comm = make_group(*this,ranks);
			LOG(INFO) << "GROUP FORMED!";
			MPI_Barrier(node_comm());

			break;
		}

		default:
			assert(false);
		}

	}

	LOG(INFO) << "Worker end";
}

} // end namespace mpits 
