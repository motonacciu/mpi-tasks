#pragma once

namespace mpits {
namespace utils {

	template <typename T>
	struct id { 
		const T& operator()(const T& t) const { return t; }
	};

} // end utils namespace 
} // end mpits namespace 
