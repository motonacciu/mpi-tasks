
#pragma once 

#include <memory>
#include <cxxabi.h>

namespace mpits {
namespace utils {

class any {
	
	struct container {
		virtual void print() const = 0;
		virtual const std::type_info& type() const = 0;

		virtual ~container() { }
	};

	template <class T>
	struct container_impl : public container {

		container_impl(const T& val) : val(val) { }
		container_impl(T&& val) : val(std::move(val)) { }

		const T& value() const { return val; }
		
		const std::type_info& type() const { return typeid(T); }

		void print() const { 
			int status;
			std::cout << abi::__cxa_demangle(typeid(T).name(), 0, 0, &status) << std::endl;
		}

		T val;
	};

	std::unique_ptr<const container> m_ptr;

public:
//	template <class T>
//	any(const T& value) : m_ptr( new container_impl<std::tuple<T>>(std::make_tuple(value)) ) 
//		{ } 
//
	//template <class T>
	//any(T&& value) : m_ptr( new container_impl<std::tuple<T>>(std::move(value)) ) { } 

	template <class... T>
	any(const T&&... value) : 
		m_ptr( new container_impl<std::tuple<T...>>( std::make_tuple(std::move(value)...) ) ) 
	{ } 

	template <class... T>
	any(T&&... value) : 
		m_ptr( new container_impl<std::tuple<T...>>( std::make_tuple(std::move(value)...) ) ) 
	{ } 

	any(any&& other) : m_ptr( std::move(other.m_ptr) ) { }

	any& operator=(any&& other) {
		m_ptr = std::move(other.m_ptr);
		return *this;
	}

	template <class T>
	const T& as() const {
		if (typeid(T) == m_ptr->type()) {
			return static_cast<const container_impl<T>*>(m_ptr.get())->value();
		}
		assert(false && "Invalid cast");
	}

};


} // end utils namespace 
} // end mpits namesapce 
