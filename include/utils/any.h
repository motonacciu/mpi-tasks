
#pragma once 

#include <memory>

namespace mpits {
namespace utils {

class any {
	
	struct container {
		virtual ~container() { }
	};

	template <class T>
	struct container_impl : public container {

		container_impl(const T& val) : val(val) { }
		container_impl(T&& val) : val(std::move(val)) { }

		const T& value() const { return val; }

		T val;
	};

	std::unique_ptr<container> m_ptr;

public:
	template <class T>
	any(const T& value) : m_ptr( new container_impl<T>(value) ) { } 

	template <class T>
	any(const T&& value) : m_ptr( new container_impl<T>(std::move(value)) ) { } 

	any(any&& other) : m_ptr( std::move(other.m_ptr) ) { }

	any& operator=(any&& other) {
		m_ptr = std::move(other.m_ptr);
		return *this;
	}

	template <class T>
	const T& as() const {
		if( const container_impl<T>* ptr = dynamic_cast<const container_impl<T>*>(m_ptr.get()))
			return ptr->value();
		assert(false && "Invalid cast");
	}

};


} // end utils namespace 
} // end mpits namesapce 
