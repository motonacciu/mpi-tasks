#pragma once 

#include "utils/queue.h"
#include "utils/logging.h"

#include <map>
#include <thread>
#include <chrono>
#include <functional>

#include "utils/any.h"

#include "comm/message.h"

namespace mpits {

struct Event { 

	enum EventType { 
		#define EVENT(EVT_ID, ...) EVT_ID,
		#include "events.def"
		#undef EVENT
	};
	
	Event(const EventType& 	evt, 
		  utils::any&&      content, 
		  utils::time_point time = std::chrono::high_resolution_clock::now()) 
	: 
		m_event_id(evt), m_time(time), m_content(std::move(content)) { }

	Event(Event&& other) :
		m_event_id(other.m_event_id), 
		m_time(other.m_time), 
		m_content(std::move(other.m_content)) { }

	Event& operator=(Event&& other) {
		m_event_id = other.m_event_id;
		m_time = other.m_time;
		m_content = std::move(other.m_content);
		return *this;
	}

	const EventType& event_id() const { return m_event_id; }

	const utils::time_point& schedule_time() const { return m_time; }

	template <class T>
	const T& content() const { 
		return m_content.as<T>(); 
	}
	
	static std::string evtToStr(EventType const& evt);

private:
	EventType  			m_event_id;
	utils::time_point 	m_time;
	utils::any 			m_content;
};


std::pair<std::vector<Event>::iterator, utils::time_point> 
TimePriorityPolicy(std::vector<Event>& queue) {

	std::vector<Event>::iterator next = queue.begin();
	utils::time_point curr_time = std::chrono::high_resolution_clock::now();

	bool found=false;
	for (auto it = queue.begin(), end = queue.end(); it!=end; ++it) {
		if (curr_time > it->schedule_time()) {
			next = it;
			curr_time = it->schedule_time();
			found = true;
		}
	}

	return { found ? next : queue.end(), curr_time };
}

typedef utils::BlockingQueue<Event, std::vector> EventQueue;


template <class... T>
struct default_filter {
	bool operator()(const T&...) { return true; }
};


struct EventHandler {

	typedef std::function<bool (const utils::any& evt)> EventHandle;
	typedef std::function<bool (const utils::any& evt)> EventFilter;
	
	typedef size_t HandleID;
		
	typedef std::tuple<HandleID, utils::any, utils::any> HandlePair;
	typedef std::shared_ptr<HandlePair> 				HandlePairPtr; 
	
	typedef std::vector<HandlePairPtr> 					HandlersList;
	typedef std::map<Event::EventType, HandlersList> 	HandleMap;
	
	typedef std::map<HandleID, Event::EventType>		HanlderRegister;

	EventHandler() :
		m_event_queue(TimePriorityPolicy), 
		m_handler_count(0) { }

	EventHandler(const EventHandler& other) = delete;
	
	template <class... T>
	HandleID connect(const Event::EventType& 					evt,
	 			 	 const std::function<bool (const T&...)>& 	handle) 
	{
		return connect(evt, handle, 
					   std::function<bool (const T&...)>(default_filter<T...>())
					  );
	}

	template <class... T>
	HandleID connect(const Event::EventType& 					evt, 
	 			 	 const std::function<bool (const T&...)>& 	handle, 
				 	 const std::function<bool (const T&...)>& 	filter ) 
	{
		std::lock_guard<std::mutex> lock(m_mutex); 
		LOG(DEBUG) << "{@EH} Connecting event lister for '" << Event::evtToStr(evt);
	
		// Search for the handle ID 
		auto fit = m_handlers.find(evt);
		if (fit == m_handlers.end()) {
			fit = m_handlers.insert( {evt, HandlersList()} ).first;
		}

		// create the Handle Pair
		auto handlePtr = std::make_shared<HandlePair>(
							++m_handler_count, 
							std::move(handle), 
							std::move(filter)
						);

		fit->second.push_back( handlePtr );
	
		m_handle_reg.insert( {m_handler_count, evt} );
		return m_handler_count;
	}

	void disconnect(HandleID const& id);

	EventQueue& queue() { return m_event_queue; }

	void operator()();

private:
	
	void process_event(Event const& evt);
	void disconnect_nts(HandleID const& id);

	std::mutex 		m_mutex;
	
	EventQueue 		m_event_queue;
	HandleMap 		m_handlers;

	HandleID 		m_handler_count;
	HanlderRegister m_handle_reg;

};

} // end tasksys namespace 

