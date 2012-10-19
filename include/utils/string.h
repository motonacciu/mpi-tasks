#pragma once 

#include "utils/functional.h"

#include <iostream>
#include <initializer_list>
#include <functional>

namespace mpits {
namespace utils {
	
	namespace {

		template <typename IterT, typename Functor>
		struct Joinable {

			typedef IterT iter_type;
		
			std::string sep;
			Functor 	func;

			Joinable(const std::string& sep, const Functor& func) : 
				sep(sep), func(func) { }

			Joinable(Joinable<IterT,Functor>&& other) : 
				sep(std::move(other.sep)), 
				func(std::move(other.func)) { }

			virtual IterT getBegin() const = 0;
			virtual IterT getEnd() const = 0;
		};


		template <typename IterT, typename Functor>
		struct StringJoinIter : public Joinable<IterT, Functor> {
			
			IterT begin, end;

			StringJoinIter(
					const std::string& 	sep, 
					const IterT& 		begin, 
					const IterT& 		end,
					const Functor&		func
				) : Joinable<IterT,Functor>(sep, func), begin(begin), end(end) { }
			
			IterT getBegin() const 	{ return begin; }
			IterT getEnd() const 	{ return end; }

			StringJoinIter(StringJoinIter&& other) :
				Joinable<IterT,Functor>(std::move(other)),
				begin( other.begin ),
				end( other.end ) { }

		private:
			StringJoinIter(const StringJoinIter& other) = delete;
		};

		template <typename Container, typename Functor>
		struct StringJoinCont: public Joinable<typename Container::const_iterator, Functor> {

			typedef typename Container::const_iterator iter_type;

			const Container cont;

			StringJoinCont(const std::string& 	sep, 
						   const Container&& 	cont,
						   const Functor& 		func) : 
				Joinable<iter_type, Functor>(sep,func), 
				cont(std::move(cont)) { }
			
			StringJoinCont(StringJoinCont&& other) :
				Joinable<iter_type, Functor>(std::move(other)),
				cont( std::move(other.cont) )
			{ }

			iter_type getBegin() const 			{ return cont.begin(); }
			iter_type getEnd() const 			{ return cont.end(); }

		private:
			StringJoinCont(const StringJoinCont& other) = delete;

		};

	} // end anonymous namespace 
	
	/**
	 * Builds a string joiner starting from a begin/end iterator, a separator 
	 * and an optional manipulation function which is applied to the element right 
	 * before they are streamed to the output stream 
	 */
	template <typename IterT, typename Functor=id<typename IterT::value_type>>
	StringJoinIter<IterT,Functor> 
		join(const IterT& begin, 
			 const IterT& end, 
			 const std::string& sep = ",", 
			 const Functor& func=Functor()) 
	{
		return StringJoinIter<IterT,Functor>(sep, begin, end, func);
	}

	template <typename Container, typename Functor=id<typename Container::value_type>>
	StringJoinIter<typename Container::const_iterator, Functor> 
		join(const Container& cont, const std::string& sep=",", const Functor& func=Functor())
	{
		return StringJoinIter<typename Container::const_iterator, Functor>(
				sep, cont.begin(), cont.end(), func
			);
	}

	template <typename Container, typename Functor=id<typename Container::value_type>>
	StringJoinCont<Container,Functor> 
		join(const Container&& cont, const std::string& sep=",", const Functor& func=Functor())
	{
		return StringJoinCont<Container,Functor>(sep, std::move(cont), func);
	}

	template <typename T, typename Functor=id<T>>
	StringJoinCont<std::vector<T>,Functor>
		join(std::initializer_list<T> cont, const std::string& sep=",", const Functor& func=Functor()) 
	{
		return join(std::vector<T>(std::move(cont)), sep, func);
	}

} // end utils namespace 
} // end mpits namespace 

namespace std {

	template <typename IterT, typename Functor>
	std::ostream& operator<<(std::ostream& out, mpits::utils::Joinable<IterT,Functor>&& join) {

		if (join.getBegin() == join.getEnd()) { return out; }
		IterT it = join.getBegin();
		out << join.func(*it);

		while(++it != join.getEnd()) { out << join.sep << join.func(*it); }
		return out;
	}

} // end std namespace 

