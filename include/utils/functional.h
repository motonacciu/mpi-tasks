#pragma once

#include <memory>

namespace mpits {
namespace utils {

	template <typename T>
	struct id { 
		const T& operator()(const T& t) const { return t; }
	};

	/** 
	 * Functor which deref the passed content 
	 * + specialization for possible pointer types 
	 */
	template <typename T>
	struct deref;

	// specialization for pointers
	template <typename T>
	struct deref<T*> {
		const T& operator()(const T* t) const { return *t; }
	};

	template <typename T>
	struct deref<std::shared_ptr<T>> {
		const T& operator()(const std::shared_ptr<T> t) const { return *t; }
	};

	template <typename T>
	struct deref<std::unique_ptr<T>> {
		const T& operator()(const std::unique_ptr<T> t) const { return *t; }
	};

} // end utils namespace 
} // end mpits namespace 
