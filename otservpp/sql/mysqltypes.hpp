#ifndef OTSERVPP_SQL_MYSQLTYPES_HPP_
#define OTSERVPP_SQL_MYSQLTYPES_HPP_

#include <mysql/mysql_com.h>

/*! \file
 * This file contains some helper templates that aid in the creation of MySql prepared
 * statements. The only public stuff here are the MySQL types that are exported in sql.hpp.
 */
namespace otservpp{ namespace sql{ namespace mysql{

template <enum_field_types T>
struct SqlType{
	enum { SqlT = T };
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


/// Fills the given MYSQL_BIND structure with the given values recursively.
/// All the other fill* functions form part of the job of this function.
template <class... Values>
inline void fillParams(MYSQL_BIND* bind, const std::tuple<Values...>& values)
{
	fill<sizeof...(Values)-1>(bind, values);
}

template <int pos, class Tuple>
inline typename std::enable_if<(pos > 0)>::value
fillParams(MYSQL_BIND* bind, const Tuple& values)
{
	fill(bind[pos], std::get<pos>(values));
	fill<pos-1>(bind, values);
}

template <int pos, class Tuple>
inline typename std::enable_if<pos == 0>::value
fillParams(MYSQL_BIND* bind, const Tuple& values)
{
	fill(bind[0], std::get<0>(values));
}

inline void fill(MYSQL_BIND& bind, Null)
{
	bind.buffer_type = MYSQL_TYPE_NULL;
};

template <class T, int SqlT = CppToSql<T>::SqlT>
inline void fill(MYSQL_BIND& bind, const T& value)
{
	bind.buffer_type = SqlT;
	fillBuffer(bind, value);
}

template <class T>
inline typename std::enable_if<std::is_scalar<T>::value>
fillBuffer(MYSQL_BIND& bind, const T& value)
{
	bind.buffer = static_cast<void*>(&value);
	bind.is_unsigned = std::is_unsigned<T>::value;
}

template <class T>
inline typename std::enable_if<std::is_floating_point<T>::value>
fillBuffer(MYSQL_BIND& bind, const T& value)
{
	bind.buffer = static_cast<void*>(&value);
}

inline void fillBuffer(MYSQL_BIND& bind, const std::string& str)
{
	bind.buffer = static_cast<void*>(str.data());
	//bind.buffer_length = str.capacity(); not needed when binding params?
	bind.length = str.length();
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


} /* namespace mysql */
} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_MYSQLTYPES_HPP_
