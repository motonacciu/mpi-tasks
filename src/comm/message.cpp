#include "comm/message.h"

namespace mpits {
namespace comm {

//==== Message ====================================================================================
Message Message::DummyMessage;
	
bool Message::operator==(const Message& other) const { 
	Bytes my_bytes = (m_pimpl?bytes():Bytes()), 
		  other_bytes = other.m_pimpl?other.bytes():Bytes();

	return  m_msg_id == other.m_msg_id 
			&& m_ep == other.m_ep 
			&& (
				(!m_pimpl && !other.m_pimpl) 
				|| ( 
					m_pimpl && other.m_pimpl && 
					size() == other.size() && 
					std::equal(my_bytes.begin(), my_bytes.end(), other_bytes.begin()) 
				   ) 
			);
}

void Message::generate_content() {
	if(!m_content) {
		// generate content
		m_content.reset(new Bytes( m_pimpl->bytes() ));
	}
	assert(m_content && "Generation of content failed");
}

std::string Message::msg_id_to_str(MessageType const& msg_id) {
	switch(msg_id) {
	#define MESSAGE(MSG_ID, ...) case MSG_ID: return #MSG_ID;
	#include "comm/message.def"
	#undef MESSAGE
	default:
		assert(false && "MessageType not declared!");
	}
}

} // end comm namespace 
} // end mpits namespace
