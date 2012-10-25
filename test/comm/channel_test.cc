#include <gtest/gtest.h>
#include "comm/message.h"
#include "utils/string.h"
#include "comm/channel.h"

#include <sstream>
#include <vector>

#include <mpi.h>

using namespace mpits;
using namespace mpits::comm;
using namespace mpits::utils;

TEST(Channel, Receive) {

	MPI_Init(NULL,NULL);
	Logger::get(std::cout, DEBUG);

	EventHandler handler;

	std::thread h(std::ref(handler));

	ReceiveChannel 	rchan(handler);
	SendChannel 	schan(handler);
	handler.queue().push( 
			Event(Event::RECV_CHN_PROBE, utils::any(10ul, std::vector<MPI_Comm>({MPI_COMM_WORLD}))) );


//	int rank;
//	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//	if (rank==0) {
//		handler.queue().push(
//			Event(Event::SEND_MSG, any(Message(Message::TEST, 1, MPI_COMM_WORLD, 1)))
//		);
//	} 
//
	sleep(2);
	handler.queue().push( Event(Event::SHUTDOWN, true) );
	h.join();

	MPI_Finalize();
}
