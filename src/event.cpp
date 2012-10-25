
#include "event.h"
#include "utils/functional.h"
#include <cassert>
#include <set>
#include <unordered_set>

#define CHECK_MPI_THREAD_LEVEL \
	int claimed;	\
	MPI_Query_thread(&claimed);	\
	assert(claimed == MPI_THREAD_MULTIPLE && "Cannot run task system on the underlying MPI library");

namespace {
// Copyied from:
// 	http://stackoverflow.com/questions/687490/how-do-i-expand-a-tuple-into-variadic-template-functions-arguments
//
template<size_t N>
struct Apply {

	template<typename F, typename T, typename... A>
	static inline auto apply(const F& f, const T& t, const A&... a)
		-> decltype(Apply<N-1>::apply(f, t, std::get<N-1>(t), a...))
	{
		return Apply<N-1>::apply(f, t, std::get<N-1>(t), a...);
	}
};

template<>
struct Apply<0> {
    template<typename F, typename T, typename... A>
    static inline auto apply(const F& f, const T&, const A&... a)
		-> decltype( f(a...) ) 
	{
        return f(a...);
    }
};

template<typename F, typename T>
inline auto apply(const F& f, const T& t) 
	-> decltype(Apply<std::tuple_size<T>::value>::apply(f,t))
{
    return Apply<std::tuple_size<T>::value>::apply(f,t);
}

} // end anonymous namespace 


namespace mpits {

std::string Event::evtToStr(Event::EventType const& evt) {
	switch(evt) {
	#define EVENT(EVT, ...) 	\
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
		[&id](const HandlePairPtr& cur){ return std::get<0>(*cur) == id; }
	);

	assert( vit != hit->second.end() );
	
	m_handle_reg.erase(fit);
	hit->second.erase(vit);
}

void EventHandler::disconnect(EventHandler::HandleID const& id) {
#ifdef USE_SYNCH
	std::lock_guard<std::mutex> lock(m_mutex); 
#endif
	disconnect_nts(id);
}

void EventHandler::process_event(Event const& evt) {
#ifdef USE_SYNCH
	std::lock_guard<std::mutex> lock(m_mutex); 
#endif
	auto fit = m_handlers.find(evt.event_id());
	
	if (fit == m_handlers.end()) { return ; }
	
	LOG(DEBUG) << "Serving event '" << Event::evtToStr(evt.event_id()) << "', " 
			   << "(number of registered handlers: " << fit->second.size() << ")";
			 
	std::unordered_set<HandleID> to_disconnect;
	
	std::for_each(fit->second.begin(), fit->second.end(), 
		[&] (HandlePairPtr const& cur) {
			switch(evt.event_id()) {
			#define EVENT(EVT_ID, ...) \
				case Event::EVT_ID: {\
				typedef std::tuple<typename utils::constify<bool, __VA_ARGS__>::type> func_type; \
				const auto& filter = std::get<0>(std::get<2>(*cur).as<func_type>()); \
				if ( apply(filter, evt.content<std::tuple<__VA_ARGS__>>()) ) { 	\
					if ( apply(std::get<0>(std::get<1>(*cur).as<func_type>()), \
						 evt.content<std::tuple<__VA_ARGS__>>()) \
					) \
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
		// LOG(DEBUG) << "{@EH} EVENT '" << Event::evtToStr(evt.event_id());

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
