#ifndef OTSERVPP_EXCEPTION_HPP_
#define OTSERVPP_EXCEPTION_HPP_

#include <boost/exception/exception.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/throw_exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/diagnostic_information.hpp>

#define OTSERVPP_THROW(x) BOOST_THROW_EXCEPTION(x)

namespace otservpp{

struct Exception : virtual std::exception, virtual boost::exception {};

template <class Tag, class Value>
using ErrorInfo = boost::error_info<Tag, Value>;

template <class T>
void BOOST_ATTRIBUTE_NORETURN throwException(const T& e)
{
	boost::throw_exception(e);
}

template <class I, class T>
auto getErrorInfo(T&& e)-> decltype(boost::get_error_info<I>(std::forward<T>(e)))
{
	return boost::get_error_info<I>(std::forward<T>(e));
}

}

#endif // OTSERVPP_EXCEPTION_HPP_
