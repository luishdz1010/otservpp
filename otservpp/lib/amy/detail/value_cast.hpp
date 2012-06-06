#ifndef __AMY_DETAIL_VALUE_CAST_HPP__
#define __AMY_DETAIL_VALUE_CAST_HPP__

#include <amy/sql_types.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace amy {
namespace detail {

template<typename ValueType>
ValueType value_cast(std::string const& str) {
    return boost::lexical_cast<ValueType>(str);
}

template<>
inline sql_tinyint value_cast(std::string const& str) {
    return boost::numeric_cast<sql_tinyint>(boost::lexical_cast<int>(str));
}

template<>
inline sql_tinyint_unsigned value_cast(std::string const& str) {
	return boost::numeric_cast<sql_tinyint_unsigned>(boost::lexical_cast<int>(str));
}

template<>
inline sql_text value_cast(std::string const& str) {
    return str;
}

template<>
inline sql_bool value_cast(std::string const& str) {
    sql_bool v;
    std::istringstream(str) >> std::boolalpha >> v;
    return v;
}

template<>
inline sql_datetime value_cast(std::string const& str) {
    using namespace boost::posix_time;

    std::istringstream in(str);
    in.unsetf(std::ios::skipws);
    in.imbue(std::locale(std::locale::classic(),
                         new time_input_facet("%Y-%m-%d %H:%M:%S%F")));

    sql_datetime v;
    in >> v;
    return v;
}

template<>
inline sql_time value_cast(std::string const& str) {
    using namespace boost::posix_time;

    // It's a pity that Boost.Date_Time cannot correctly parse negative time
    // durations, so we have to deal with the sign here.
    bool negative = (str[ 0 ] == '-') ? true : false;
    std::string const* ptr = &str;
    std::string s;
    if (negative) {
        s = str.substr(1);
        ptr = &s;
    }

    time_input_facet* input_facet = new time_input_facet();
    input_facet->time_duration_format("%H:%M:%S:%F");

    std::istringstream in(*ptr);
    in.unsetf(std::ios::skipws);
    in.imbue(std::locale(std::locale::classic(), input_facet));

    sql_time v;
    in >> v;

    if (negative) {
        v = hours(0) - v;
    }

    return v;
}

}   //  namespace detail
}   //  namespace amy

#endif  // __AMY_DETAIL_VALUE_CAST_HPP__

// vim:ft=cpp ts=4 sw=4 et
