
#include "event.h"
#include <cassert>
#include <set>
#include <unordered_set>

#define CHECK_MPI_THREAD_LEVEL \
	int claimed;	\
	MPI_Query_thread(&claimed);	\
	assert(claimed == MPI_THREAD_MULTIPLE && "Cannot run task system on the underlying MPI library");

namespace mpits {

std::string Event::evtToStr(Event::EventType const& evt) {
	switch(evt) {
	#define EVENT(EVT, TYPE) 	\
	case EVT:	return #EVT;
	#include "events.def"
	#undef EVENT
	default:
		LOG(ERROR) << "{@EH} Event '" << evt << "' not supported";
		assert(false && "Event not supported");
	}
}
	
void EventHandler::disconnect_nts(HandleID const& id) {
	auto fit = m_handle_reg.find(id);
	assert( fit != m_handle_reg.end() );

	// find the HandlePair associated to this handler ID
	auto hit = m_handlers.find(fit->second);
	assert(hit != m_handlers.end());
	
	auto vit = std::find_if(hit->second.begin(), hit->second.end(), 
		[&id](HandlePairPtr const& cur){ return std::get<0>(*cur) == id; }
	);

	assert( vit != hit->second.end() );
	
	m_handle_reg.erase(fit);
	hit->second.erase(vit);
}

void EventHandler::disconnect(EventHandler::HandleID const& id) {
	std::lock_guard<std::mutex> lock(m_mutex); 
	disconnect_nts(id);
}

void EventHandler::process_event(Event const& evt) {
	std::lock_guard<std::mutex> lock(m_mutex); 
	auto fit = m_handlers.find(evt.event_id());
	
	if (fit == m_handlers.end()) { return ; }
	
	LOG(DEBUG) << "Serving event '" << Event::evtToStr(evt.event_id()) << "', " 
			   << "(number of registered handlers: " << fit->second.size() << ")";
			 
	std::unordered_set<HandleID> to_disconnect;
	
	std::for_each(fit->second.begin(), fit->second.end(), 
		[&] (HandlePairPtr const& cur) {
			switch(evt.event_id()) {
			#define EVENT(EVT_ID, EVT_TYPE) \
				case Event::EVT_ID: {\
				auto& filter = \
					boost::any_cast<std::function<bool (const EVT_TYPE&)>&>( \
						std::get<2>(*cur) \
					); \
				if ( filter( evt.content<EVT_TYPE>() ) ) { 	\
					if ( boost::any_cast< \
					  std::function<bool (const EVT_TYPE&)>&\
					 >(std::get<1>(*cur))( evt.content<EVT_TYPE>() ) )	\
						to_disconnect.insert( std::get<0>(*cur) ); \
				} \
				break; \
				}
			#include "events.def"
			#undef EVENT
			}
		});
	
	std::for_each(to_disconnect.begin(), to_disconnect.end(), 
			[&](HandleID const& curr){ disconnect_nts(curr); });
}


void EventHandler::operator()() {
	// CHECK_MPI_THREAD_LEVEL;
	LOG(DEBUG) << "{@EH} Starting event handler thread\\";
#ifdef USE_THREAD_POOL
	LOG(DEBUG) << "{@EH} Using threadpool";
	// create the threadpool of threads used to server requests 
	boost::threadpool::pool pool(1);
#endif
	while(true) {
		// LOG(DEBUG) << "{@EH} Waiting for event";
		Event&& evt = m_event_queue.pop();
		LOG(DEBUG) << "{@EH} EVENT '" << Event::evtToStr(evt.event_id());

		if (evt.event_id()==Event::SHUTDOWN) { break; }
#ifdef USE_THREAD_POOL
		pool.schedule( std::bind(&EventHandler::process_event, std::ref(*this), evt) );
#else 
		process_event(evt);
#endif
	}
	LOG(DEBUG) << "\\{EH@} Terminating event handler thread";
}


} // end tasksys namespace 
