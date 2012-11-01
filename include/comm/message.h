#pragma once 

#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <sstream>

#include <mpi.h>

#include <boost/serialization/vector.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

namespace boost {
namespace serialization {

template<uint N>
struct Serialize
{
    template<class Archive, typename... Args>
    static void serialize(Archive& ar, std::tuple<Args...>& t, unsigned int version) {
        ar & std::get<N-1>(t);
        Serialize<N-1>::serialize(ar, t, version);
    }
};

template<>
struct Serialize<0>
{
    template<class Archive, typename... Args>
    static void serialize(Archive& ar, std::tuple<Args...>& t, unsigned int version) {}
};

template<class Archive, typename... Args>
void serialize(Archive & ar, std::tuple<Args...> & t, unsigned int version) {
    Serialize<sizeof...(Args)>::serialize(ar, t, version);
}

} // end serialization namespace 
} // end boost namespace 


namespace mpits {
namespace comm {

typedef unsigned char Byte;
typedef std::vector<Byte> Bytes;

static const int all = -1;

template <class T>
struct msg_content_traits {

	static Bytes to_bytes(const T& v) { 
		std::ostringstream ss;
    	boost::archive::text_oarchive oa(ss);
    	oa << v;
    	std::string str = ss.str();
    	return std::move(Bytes(str.begin(), str.end()));
	}

	static T from_bytes(const Bytes& bytes) {
		std::istringstream iss( std::string(bytes.begin(), bytes.end()) );
		T ret;
		boost::archive::text_iarchive ia(iss);
		ia >> ret;
		return std::move(ret);
	}

};

/**************************************************************************************************
 * Message abstraction: messages are exchanged between schedulers via MPI channel communication. 
 *************************************************************************************************/
class Message {
	
public:
	enum MessageType {
	#define MESSAGE(MSG_ID, ...)	MSG_ID,
	#include "comm/message.def"
	#undef MESSAGE
	};

	static std::string msg_id_to_str(const MessageType& msg_id);

private:
	MessageType m_msg_id;
	int 		m_ep;
	MPI_Comm 	m_comm;
	
	// Interface for a generic message content 
	struct content {
		virtual Bytes bytes() const = 0;
	};
	
	// Templates holder class for a generic content
	template <class Content> 
	struct content_impl : public content { 
		
		content_impl(const Content& c) : content(c) { }
		
		Bytes bytes() const { 
			return msg_content_traits<Content>::to_bytes(content); 
		}
		
		Content content;
	};
	
	std::unique_ptr<content> m_pimpl;
	std::unique_ptr<Bytes>   m_content;
	
	Message(): m_comm(MPI_COMM_WORLD) { }

	Message(const Message&) = delete;

	void generate_content();

public:
	
	static Message DummyMessage;

	template <class Content>
	Message(const MessageType& id, int ep, MPI_Comm comm, const Content& content) : 
		m_msg_id(id), 
		m_ep(ep), 
		m_comm(comm), 
		m_pimpl( new content_impl<Content>(content) ) 
	{ 
		generate_content(); 
	}

	Message(Message&& other) :
		m_msg_id(other.m_msg_id), 
		m_ep(other.m_ep),
		m_comm(other.m_comm),
		m_pimpl( std::move(other.m_pimpl) ),
		m_content( std::move(other.m_content) ) { }
	
	inline const MessageType& msg_id() const { return m_msg_id; } 
	
	inline int endpoint() const { return m_ep; }
	
	inline MPI_Comm comm() const { return m_comm; }
	
	inline size_t size() const { return m_content->size(); }
	
	inline Bytes bytes() const { return *m_content; }
	
	template <class Content>
	Content get_content_as() const {
		content_impl<Content>& cc = dynamic_cast<content_impl<Content>&>(*m_pimpl);
		return cc.content;
	}
	
	bool operator==(const Message& other) const;
};

} // end comm namespace 
} // end mpits namespace 

