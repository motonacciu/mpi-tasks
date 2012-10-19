#pragma once

#include <thread>
#include <condition_variable>

#include <deque>
#include <algorithm>

namespace mpits {
namespace utils {

template <class T, template <typename...> class Cont=std::deque>
typename Cont<T>::iterator FIFOPolicy(Cont<T>& queue) {
	return queue.begin();
}

/**
 * This is an implementation of a blocking queue
 */
template <class ValT, template <typename...> class Cont=std::deque>
class BlockingQueue {

	BlockingQueue(const BlockingQueue&) = delete;

public:
	typedef Cont<ValT>					Queue;
	typedef typename Queue::iterator 	QueueIterator;
	
private:
	Queue			 		m_queue;
	std::mutex				m_mutex;
	std::condition_variable	m_condition;
	
	// Function prototype for selection policies which are applied to 
	// select the next element to be pulled out from the queue. 
	typedef std::function<QueueIterator (Queue&)> SelectionPolicy;
	SelectionPolicy m_sel_policy;

public:
		
	BlockingQueue(const SelectionPolicy& policy=FIFOPolicy<ValT,Cont>) : 
		m_sel_policy(policy) { }

	/*
	 * Insert an element into the queue's head, this method is non-blocking
	 * as far as a max dimension for this queue is not specified.
	 */
	void push(ValT const& val);

	/*
	 * Extract one element from the queue's head, if the queue is empty this method
	 * blocks the consumer until one element is pushed by the producer
	 */
	ValT pop();
	
	size_t size() {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.size();
	}

	bool contains(ValT const& val) {
		std::unique_lock<std::mutex> lock(m_mutex);

		return std::find(m_queue.begin(), m_queue.end(), val) != m_queue.end();
	}
	
	bool contains_if(std::function<bool (ValT const&)> const& func) {
		std::unique_lock<std::mutex> lock(m_mutex);

		return std::find_if(m_queue.begin(), m_queue.end(), func) != m_queue.end();
	}
	
	std::pair<bool, ValT> pop_if(std::function<bool (ValT const&)> const& func) {
		std::lock_guard<std::mutex> lock(m_mutex);

		auto fit = std::find_if(m_queue.begin(), m_queue.end(), func);
		if ( fit != m_queue.end() ) {
			ValT val = *fit;
			m_queue.erase(fit);
			return std::make_pair(true, val);
		}
		return std::make_pair(false, ValT());
	}
	
	bool empty() {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.empty();
	}

	void set_policy(SelectionPolicy const& policy) { 
		std::lock_guard<std::mutex> lock(m_mutex);
		// Set the new policy
		m_sel_policy = policy;
		// wake up blocked thread
		m_condition.notify_one(); 
	}

	void notify() {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_condition.notify_one(); 
	}

};

template <class ValT, template <typename...> class Cont>
inline void BlockingQueue<ValT,Cont>::push(ValT const& val) {
	// the lock guarantee the mutual exclusion execution
	std::lock_guard<std::mutex> lock(m_mutex);

	size_t initial_size = m_queue.size();
	m_queue.push_back(val);
	assert(initial_size+1 == m_queue.size());
	m_condition.notify_one(); // notify consumers
}

template <class ValT, template <typename...> class Cont>
inline ValT BlockingQueue<ValT,Cont>::pop() {
	// the lock guarantee the mutual exclusion execution
	std::unique_lock<std::mutex> lock(m_mutex);
	QueueIterator iter = m_sel_policy(m_queue);

	while (m_queue.empty() || iter == m_queue.end()) {
		m_condition.wait(lock); // consumers are blocked
		iter = m_sel_policy(m_queue);
	}

	assert( iter != m_queue.end() );
	size_t initial_size = m_queue.size();
	ValT ret = *iter;
	m_queue.erase(iter);
	assert(initial_size-1 == m_queue.size());
	return ret;
}

} // end utils namespace 
} // end mpits namespace 
