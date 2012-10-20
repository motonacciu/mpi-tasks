#pragma once 

#include <mpi.h>

#include <cassert>

#include <vector>
#include "message.h"
#include "event.h"

namespace mpits {
namespace comm { 

class SendChannel {


};


class ReceiveChannel {
	
	EventQueue& m_queue;

public:

	ReceiveChannel(EventHandler& evt) : m_queue(evt.queue()) 
	{
		evt.connect(
			Event::RECV_CHN_PROBE, 
			std::function<bool (const std::vector<MPI_Comm>&)>(std::ref(*this))
		);
	}
	
	bool operator()(const std::vector<MPI_Comm>& comms) {

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
			
				//m_queue.push( Event(Event::MSG_RECVD, std::move(msg)) );
			}

		}
		return false;
	}

};

} // end comm namespace 
} // end mpits namespace 
