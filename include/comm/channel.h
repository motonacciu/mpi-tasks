#pragma once 

#include <mpi.h>

#include <cassert>

#include <vector>
#include "message.h"
#include "event.h"

namespace mpits {
namespace comm { 

struct SendChannel {

	SendChannel(EventHandler& evt) 
	{
		evt.connect(
			Event::SEND_MSG, 
			std::function<bool (const Message& msg)>(std::ref(*this))
		);
	}
	
	bool operator()(const Message& msg);

};


class ReceiveChannel {
	
	EventQueue& m_queue;
	bool shutdown;

public:

	ReceiveChannel(EventHandler& evt) : m_queue(evt.queue()), shutdown(false) 
	{
		evt.connect(
			Event::RECV_CHN_PROBE, 
			std::function<bool (const size_t&, const std::vector<MPI_Comm>&)>(std::ref(*this))
		);

		evt.connect(
			Event::SHUTDOWN, 
			std::function<bool (const bool&)>(std::ref(*this))
		);

	}

	bool operator()(bool) { shutdown = true; }
	bool operator()(const size_t& delay, const std::vector<MPI_Comm>& comms);

};

} // end comm namespace 
} // end mpits namespace 
