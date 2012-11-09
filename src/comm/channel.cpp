
#include "comm/channel.h"

#include <algorithm>

#define MAX_DELAY 300ul

namespace mpits {
namespace comm {

	bool SendChannel::operator()(const Message& msg) {
			
		MPI_Send(const_cast<unsigned char*>(&msg.bytes().front()), 
			 msg.size(), MPI_BYTE, msg.endpoint(), msg.msg_id(), msg.comm()
		);

		return false;
	}

	bool ReceiveChannel::operator()(const size_t& delay, const std::vector<MPI_Comm>& comms) {

		bool received = false;
		for(MPI_Comm cur : comms) {

			MPI_Status status;

			// LOG(DEBUG) << "{@MR} Probing for message";
			int flag=0, size=0;
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, cur, &flag, &status);
		
			if (flag != 0) {
				received = true;

				MPI_Get_count( &status, MPI_BYTE, &size );
				LOG(DEBUG) << "{@MR} Receiving message of size: " << size << " (bytes)";

				Bytes buff(size);
				MPI_Recv(&buff.front(), size, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, 
						 cur, MPI_STATUS_IGNORE);

				Message msg = [&] () -> Message {
					switch(status.MPI_TAG) {
					#define MESSAGE(MSG_ID, ...) \
					case Message::MSG_ID: \
						return Message(Message::MSG_ID, \
									   status.MPI_SOURCE, \
									   cur, \
									   msg_content_traits<std::tuple<__VA_ARGS__>>::from_bytes(buff) \
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

		if (!shutdown) {
			size_t new_delay = received ? 2 : std::min(delay*2, MAX_DELAY);

			m_queue.push( 
				Event(Event::RECV_CHN_PROBE, 
					utils::any(std::move(new_delay), std::vector<MPI_Comm>(comms)), 
					std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(new_delay)
				) 
			);
		}


		return false;
	}

} // end comm namespace 
} // end mpits namespace 

