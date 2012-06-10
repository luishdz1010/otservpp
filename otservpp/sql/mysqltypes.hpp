#ifndef OTSERVPP_SQL_MYSQLTYPES_HPP_
#define OTSERVPP_SQL_MYSQLTYPES_HPP_

#include <string>
#include <vector>
#include <tuple>
#include <mysql/mysql_com.h>

/*! \file
 * This file contains some helper templates that aid in the creation of MySql prepared
 * statements. The only public stuff here are the MySQL types that are exported in sql.hpp.
 */
namespace otservpp{ namespace sql{ namespace mysql{

template <class... T>
using Row = std::tuple<T...>;

template <class... T>
using RowSet = std::vector<Row<T...>>;

struct Null{
	constexpr explicit operator bool() { return false; }
};

template <enum_field_types T>
struct SqlType{
	static const enum_field_types SqlT = T;
};

template <class T> struct CppToSql;

template <> struct CppToSql<signed char>		: public SqlType<MYSQL_TYPE_TINY> {};
template <> struct CppToSql<unsigned char> 		: public SqlType<MYSQL_TYPE_TINY> {};
template <> struct CppToSql<signed short> 		: public SqlType<MYSQL_TYPE_SHORT> {};
template <> struct CppToSql<unsigned short> 	: public SqlType<MYSQL_TYPE_SHORT> {};
template <> struct CppToSql<signed int> 		: public SqlType<MYSQL_TYPE_LONG> {};
template <> struct CppToSql<unsigned int> 		: public SqlType<MYSQL_TYPE_LONG> {};
template <> struct CppToSql<signed long long>	: public SqlType<MYSQL_TYPE_LONGLONG> {};
template <> struct CppToSql<unsigned long long> : public SqlType<MYSQL_TYPE_LONGLONG> {};
template <> struct CppToSql<float> 				: public SqlType<MYSQL_TYPE_FLOAT>{};
template <> struct CppToSql<double> 			: public SqlType<MYSQL_TYPE_DOUBLE>{};

template <> struct CppToSql<std::string>		: public SqlType<MYSQL_TYPE_STRING> {};


/// Tiny class for holding null-able values, similar to boost::optional
template <class T, class SqlT = CppToSql<T>>
struct Wrapper : public SqlT{
public:
	Wrapper(){}

	Wrapper(const Null&){}

	template <class U, class... V>
	Wrapper(U&& u, V&&... v) :
		value(std::forward<U>(u), std::forward<V>(v)...),
		null(false)
	{}

	Wrapper(const Wrapper&) = default;
	Wrapper(Wrapper&&) = default;

	bool isNull()
	{
		return null;
	}

	const T& get() const
	{
		return value;
	}

	T& get()
	{
		return value;
	}

	template <class U>
	auto operator=(U&& u)-> decltype(std::declval<T>().operator=(std::forward<U>(u)))
	{
		null = false;
		return value.operator=(std::forward<U>(u));
	}

	const Null& operator=(const Null& n)
	{
		null = true;
		return n;
	}

	explicit operator bool()
	{
		return !null;
	}

	explicit operator T()
	{
		return value;
	}

private:
	T value;
	my_bool null = true;
};


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


struct BindOutReg{
	my_bool error;
	my_bool null;
	unsigned long length;
};


template <class T>
inline typename std::enable_if<std::is_integral<T>::value>::type
bindBuffer(MYSQL_BIND& bind, const T& value)
{
	bind.buffer = (void*)&value;
	bind.is_unsigned = std::is_unsigned<T>::value;
}

template <class T>
inline typename std::enable_if<std::is_floating_point<T>::value>::type
bindBuffer(MYSQL_BIND& bind, const T& value)
{
	bind.buffer = (void*)&value;
}

inline void bindBuffer(MYSQL_BIND& bind, const std::string& str)
{
	bind.buffer = (void*)str.data();
	bind.buffer_length = str.length();
}

// string/blob buffer will be fetched after we know the actual size
inline void bindBuffer(MYSQL_BIND& bind, std::string&)
{
	bind.buffer = 0;
	bind.buffer_length = 0;
}


inline void bindOne(MYSQL_BIND& bind, const Null&) // don't allow bind Null's on results
{
	bind.buffer_type = MYSQL_TYPE_NULL;
}

template <class T, enum_field_types SqlT = CppToSql<T>::SqlT>
inline void bindOne(MYSQL_BIND& bind, const T& value)
{
	bind.buffer_type = SqlT;
	bindBuffer(bind, value);
}

template <class T, enum_field_types SqlT = CppToSql<T>::SqlT>
inline void bindOne(MYSQL_BIND& bind, T& value)
{
	bind.buffer_type = SqlT;
	bindBuffer(bind, value);
}

template <class T>
inline void bindOne(MYSQL_BIND& bind, const Wrapper<T>& value)
{
	if(value.isNull())
		bind.buffer_type = MYSQL_TYPE_NULL;
	else
		bindOne(bind, value.get());
}

// result wrappers are always null because we are actually filling them
template <class T>
inline void bindOne(MYSQL_BIND& bind, Wrapper<T>& value)
{
	bindOne(bind, value.get());
}

inline void bindOne(MYSQL_BIND& bind, const Blob& blob)
{
	bind.buffer_type = Blob::SqlT;
	bindBuffer(bind, blob.get());
}

inline void bindOne(MYSQL_BIND& bind, Blob& blob)
{
	bind.buffer_type = Blob::SqlT;
	bindBuffer(bind, blob.get());
}


template <class T>
inline void bindOneResult(MYSQL_BIND& bind, T& result, BindOutReg& reg)
{
	bind.error = &reg.error;
	bind.length = &reg.length;
	bind.is_null = &reg.null;
	bindOne(bind, result);
}


template<class T>
inline void bindOneResultBuffer(MYSQL_BIND& bind, T& result)
{
	bindBuffer(bind, result);
}

template<class T>
inline void bindOneResultBuffer(MYSQL_BIND& bind, Wrapper<T>& result)
{
	bindBuffer(bind, result.get());
}


template <int pos, class Tuple>
inline typename std::enable_if<pos == 0>::type
bindParams(MYSQL_BIND* bind, const Tuple& params)
{
	bindOne(bind[0], std::get<0>(params));
}

template <int pos, class Tuple>
inline typename std::enable_if<(pos > 0)>::type
bindParams(MYSQL_BIND* bind, const Tuple& params)
{
	bindOne(bind[pos], std::get<pos>(params));
	bindOne<pos-1>(bind, params);
}


template <int pos, class Tuple>
inline typename std::enable_if<pos == 0>::type
bindResult(MYSQL_BIND* bind, Tuple& result, BindOutReg* reg)
{
	bindOneResult(bind[0], std::get<0>(result), reg[0]);
}

template <int pos, class Tuple>
inline typename std::enable_if<(pos > 0)>::type
bindResult(MYSQL_BIND* bind, Tuple& result, BindOutReg* reg)
{
	bindOneResult(bind[pos], std::get<pos>(result), reg[pos]);
	bindResult<pos-1>(bind, result, reg);
}


// these are used for loop-binding, we only need to update the buffers
template <int pos, class Tuple>
inline typename std::enable_if<pos == 0>::type
bindResultBuffer(MYSQL_BIND* bind, Tuple& result)
{
	bindOneResultBuffer(bind[0], std::get<0>(result));
}

template <int pos, class Tuple>
inline typename std::enable_if<(pos > 0)>::type
bindResultBuffer(MYSQL_BIND* bind, Tuple& result)
{
	bindOneResultBuffer(bind[pos], std::get<pos>(result));
	bindResultBuffer<pos-1>(bind, result);
}

// non-wrapper types cannot be null, and if a truncation error occurs we must report, otherwise
// do nothing as theyre already converted
template <int col, class T>
inline bool storeOneResult(MYSQL_BIND&, MYSQL_STMT*, BindOutReg& reg, T&)
{
	return !reg.null && !reg.error;
}

// string can be null without the need for a wrapper
template <int col>
inline bool storeOneResult(MYSQL_BIND& bind, MYSQL_STMT* stmt, BindOutReg& reg, std::string& str)
{
	if(reg.length > 0){
		std::vector<char> buffer(reg.length);
		bind.buffer = (void*)&buffer[0];
		bind.buffer_length = reg.length;

		if(mysql_stmt_fetch_column(stmt, &bind, col, 0))
			return false;

		str.assign(buffer.begin(), buffer.end());
	}

	return true;
}

template <int col, class T>
inline bool storeOneResult(MYSQL_BIND& b, MYSQL_STMT* s, BindOutReg& reg, Wrapper<T>& value)
{
	return reg.null? !(value=Null()) : storeOneResult(b, s, reg, value);
}


template <int pos, class Tuple>
inline typename std::enable_if<pos == 0, bool>::type
storeResult(MYSQL_BIND* bind, MYSQL_STMT* stmt, BindOutReg* reg, Tuple& result)
{
	return storeOneResult<0>(bind[0], stmt, reg[0], std::get<0>(result));
}

template <int pos, class Tuple>
inline typename std::enable_if<(pos > 0), bool>::type
storeResult(MYSQL_BIND* bind, MYSQL_STMT* stmt, BindOutReg* reg, Tuple& result)
{
	if(!storeOneResult<pos>(bind[pos], stmt, reg[pos], std::get<pos>(result)))
		return false;
	return storeResult<pos-1>(bind, stmt, reg, result);
}

// mysql_bind_[param|result] helpers follow
template <int size>
struct Bind{
	MYSQL_BIND it[size];
	Bind()
	{
		memset(it, 0, sizeof(it));
	}

	MYSQL_BIND* get()
	{
		return it;
	}
};

template <class... T> struct BindInHelper;

template <class... In>
struct BindInHelper<Row<In...>> : public Bind<sizeof...(In)>{
	BindInHelper(const Row<In...>* params)
	{
		bindParams<sizeof...(In)-1>(this->it, *params);
	}
};

template <int size>
struct BindOutHelperBase : public Bind<size>{
	BindOutReg reg[size];

	BindOutHelperBase()
	{
		memset(reg, 0, sizeof(reg));
	}
};

template <class... T> struct BindOutHelper;

template <class... Out>
struct BindOutHelper<Row<Out...>> : public BindOutHelperBase<sizeof...(Out)>{
	enum{ size = sizeof...(Out) };
	Row<Out...>& result;

	BindOutHelper(Row<Out...>* res) :
		result(*res)
	{
		bindResult<size-1>(this->it, result, this->reg);
	}

	bool store(MYSQL_STMT* stmt)
	{
		return storeResult<size-1>(this->it, stmt, this->reg, result);
	}
};

template <class... Out>
struct BindOutHelper<RowSet<Out...>> : public BindOutHelperBase<sizeof...(Out)>{
	enum{ size = sizeof...(Out) };

	Row<Out...> holder;
	RowSet<Out...>& result;

	BindOutHelper(RowSet<Out...>* res) :
		result(*res)
	{
		bindResult<size-1>(this->it, holder, this->reg);
	}

	bool store(MYSQL_STMT* stmt)
	{
		auto& row = *result.emplace(result.end());
		bindResultBuffer<size-1>(this->it, row);

		return storeResult<size-1>(this->it, stmt, this->reg, row);
	}
};

} /* namespace mysql */
} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_MYSQLTYPES_HPP_
