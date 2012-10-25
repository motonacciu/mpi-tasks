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
	
	bool operator()(const Message& msg) {
		
		LOG(DEBUG) << "sending";
		MPI_Send(const_cast<unsigned char*>(&msg.bytes().front()), 
			 msg.size(), 
			 MPI_BYTE, 
			 msg.endpoint(), 
			 msg.msg_id(), 
			 msg.comm()
		);

		return false;

	}

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
	
	bool operator()(const size_t& delay, const std::vector<MPI_Comm>& comms) {

		LOG(DEBUG) << "DELAY:" << delay;

		for(MPI_Comm cur : comms) {

			MPI_Status status;

			int flag=0, size=0;
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, cur, &flag, &status);
		
			if (flag != 0) {
				MPI_Get_count( &status, MPI_BYTE, &size );
				LOG(DEBUG) << "{@MR} Receiving message of size: " << size << " (bytes)";

				Bytes buff(size);
				MPI_Recv(&buff.front(), size, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, 
					 cur, MPI_STATUS_IGNORE);

				Message msg = [&] () -> Message {
					switch(status.MPI_TAG) {
					#define MESSAGE(MSG_ID, CONTENT_TYPE) \
					case Message::MSG_ID: \
						return Message(Message::MSG_ID, \
									   status.MPI_SOURCE, \
									   cur, \
							  		   msg_content_traits<CONTENT_TYPE>::from_bytes(buff)\
							   );
					#include "comm/message.def"
					#undef MESSAGE
					default:
						LOG(ERROR) << std::string(buff.begin(), buff.end());
						assert(false && "Message ID unknown");
					}
				}();
			
				m_queue.push( Event(Event::MSG_RECVD, utils::any(std::move(msg))) );

			}

			
		}
		if (!shutdown)
			m_queue.push( Event(Event::RECV_CHN_PROBE, 
					utils::any(delay+50, std::vector<MPI_Comm>(comms)), 
					std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(delay+50)
			) );

		return false;
	}

};

} // end comm namespace 
} // end mpits namespace 
