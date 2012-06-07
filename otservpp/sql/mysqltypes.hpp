#ifndef OTSERVPP_SQL_MYSQLTYPES_HPP_
#define OTSERVPP_SQL_MYSQLTYPES_HPP_

#include <mysql/mysql_com.h>
#include <string>

/*! \file
 * This file contains some helper templates that aid in the creation of MySql prepared
 * statements. The only public stuff here are the MySQL types that are exported in sql.hpp.
 */
namespace otservpp{ namespace sql{ namespace mysql{

template <enum_field_types T>
struct SqlType{
	static const enum_field_types SqlT = T;
};

template <class T> struct CppToSql;

template <> struct CppToSql<char> 				: public SqlType<MYSQL_TYPE_TINY> {};
template <> struct CppToSql<unsigned char> 		: public SqlType<MYSQL_TYPE_TINY> {};
template <> struct CppToSql<short> 				: public SqlType<MYSQL_TYPE_SHORT> {};
template <> struct CppToSql<unsigned short> 	: public SqlType<MYSQL_TYPE_SHORT> {};
template <> struct CppToSql<int> 				: public SqlType<MYSQL_TYPE_LONG> {};
template <> struct CppToSql<unsigned int> 		: public SqlType<MYSQL_TYPE_LONG> {};
template <> struct CppToSql<long long> 			: public SqlType<MYSQL_TYPE_LONGLONG> {};
template <> struct CppToSql<unsigned long long> : public SqlType<MYSQL_TYPE_LONGLONG> {};

template <> struct CppToSql<float> 				: public SqlType<MYSQL_TYPE_FLOAT>{};
template <> struct CppToSql<double> 			: public SqlType<MYSQL_TYPE_DOUBLE>{};

template <> struct CppToSql<std::string>		: public SqlType<MYSQL_TYPE_STRING> {};


template <class T, class SqlT = CppToSql<T>>
struct Wrapper : public SqlT{
public:
	template <class... U>
	Wrapper(U&&... u) :
		value(std::forward<U>(u)...)
	{}

	template <class U>
	auto operator=(U&& u)-> decltype(std::declval<U>().operator=(std::forward<U>(u)))
	{
		return value.operator=(std::forward<U>(u));
	}

	const T& get() const
	{
		return value;
	}

	T& get()
	{
		return value;
	}

	explicit operator T()
	{
		return value;
	}

private:
	T value;
};


struct Null{};

typedef Wrapper<char> TinyInt;
typedef Wrapper<unsigned char> UTinyInt;
typedef Wrapper<short> SmallInt;
typedef Wrapper<unsigned short> USmallInt;
typedef Wrapper<int> Int;
typedef Wrapper<unsigned int> Uint;
typedef Wrapper<long long> BigInt;
typedef Wrapper<unsigned long long> UBigInt;

typedef Wrapper<float> Float;
typedef Wrapper<double> Double;

typedef Wrapper<std::string> String;
typedef Wrapper<std::string, SqlType<MYSQL_TYPE_BLOB>> Blob;


template <class T>
inline typename std::enable_if<std::is_scalar<T>::value>::type
fillBuffer(MYSQL_BIND& bind, const T& value)
{
	bind.buffer = static_cast<void*>(&value);
	bind.is_unsigned = std::is_unsigned<T>::value;
}

template <class T>
inline typename std::enable_if<std::is_floating_point<T>::value>::type
fillBuffer(MYSQL_BIND& bind, const T& value)
{
	bind.buffer = static_cast<void*>(&value);
}

inline void fillBuffer(MYSQL_BIND& bind, const std::string& str)
{
	bind.buffer = static_cast<void*>(const_cast<char*>(str.data()));
	// ok, mysql C API is completely retarded, for some reason we need to specify the buffer
	// length as a pointer, if we do that we will need some helper structs for this tiny stupid
	// thing. But here's the catch: it appears that if you left length as 0, buffer_length
	// is used instead, this doesn't really matter for us because we always re-bind on every
	// execution, so we will left it as is
	bind.buffer_length = str.length();//str.capacity();
	//bind.length = str.length();
}

inline void fill(MYSQL_BIND& bind, Null)
{
	bind.buffer_type = MYSQL_TYPE_NULL;
}

template <class T, enum_field_types SqlT = CppToSql<T>::SqlT>
inline void fill(MYSQL_BIND& bind, const T& value)
{
	bind.buffer_type = SqlT;
	fillBuffer(bind, value);
}

template <class T>
inline void fill(MYSQL_BIND& bind, const Wrapper<T>& value)
{
	fill(bind, value.get());
}

inline void fill(MYSQL_BIND& bind, const Blob& blob)
{
	bind.buffer_type = Blob::SqlT;
	fillBuffer(bind, blob.get());
}

template <int pos, class Tuple>
inline typename std::enable_if<pos == 0>::type
fillParams(MYSQL_BIND* bind, const Tuple* values)
{
	fill(bind[0], std::get<0>(*values));
}

template <int pos, class Tuple>
inline typename std::enable_if<(pos > 0)>::type
fillParams(MYSQL_BIND* bind, const Tuple* values)
{
	fill(bind[pos], std::get<pos>(*values));
	fillParams<pos-1>(bind, values);
}

/// Fills the given MYSQL_BIND structure with the given values recursively.
/// All the other fill* functions form part of the job of this function.
template <class... Values>
inline void fillParams(MYSQL_BIND* bind, const std::tuple<Values...>* values)
{
	fillParams<sizeof...(Values)-1>(bind, values);
}

} /* namespace mysql */
} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_MYSQLTYPES_HPP_
