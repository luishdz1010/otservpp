#ifndef OTSERVPP_DATABASE_H_
#define OTSERVPP_DATABASE_H_

#include <set>
#include <vector>
#include <deque>
#include <ostream>
#include <functional>
#include <boost/variant.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <amy/connector.hpp>
#include "../forwarddcl.hpp"


namespace otservpp { namespace sql{

struct ConnectionInfo{
	std::string schema;
	std::string user;
	std::string password;
	boost::asio::ip::tcp::endpoint endpoint;
	int flags = 0;
};

/*! Delete functor for returning unused connections to the connection pool in Database.
 * Instances of this structure are passed as the Deleter objects to any given ConnectionPtr,
 * whenever a ConnectionPtr goes out of scope, the underlying connection is returned to the
 * Database's connection pool instead of being deleted.If the database has already been deleted
 * by the time the ConnectionPtr goes out of scope, the Connection is delete.
 * \note This functor is thread-safe
 */
struct OnDeleteReturnToPool{
	std::weak_ptr<Database> wdb;

	OnDeleteReturnToPool(const DatabasePtr& db) :
		wdb(db)
	{}

	void operator()(Connection* conn)
	{
		if(auto db = wdb.lock())
			db->returnToPool(conn);
		else
			delete conn;
	}
};

class Database : public std::enable_shared_from_this{
public:
	Database(boost::asio::io_service& ioService_, const ConnectionInfo& connInfo_);

	/// Returns a QueryPtr that can be used to query this database
	/// \note This function is thread-safe
	template <class... Params, class... Args>
	Query makeQuery(Args&&... args)
	{
		return makeQuery<Params...>(&this, std::forward<Args>(args)...);
	}

	/*! Obtains a Connection from the connection pool and passes it to the given handler
	 * The handler might be executed during in the body of this function if:
	 * \li There are at least one connection left in the pool
	 * \li There's no other thread using this Database public interface
	 * If these requirements can't be meet, the given handler is stored and executed as soon as
	 * possible.
	 * \note This function is thread-safe
	 */
	template <class Handler>
	void getConnection(Handler&& handler)
	{
		strand.dispatch([=]{
			if(connectionPool.empty()){
				requestQueue.emplace_back(std::forward<Hanlder>(handler));
			} else {
				auto conn = makeConnectionPtr(connectionPool.front().release());
				connectionPool.pop_front();
				handler(std::move(conn));
			}
		});
	}

	template <class Handler>
	void runQuery(const std::string& statement)
	{

	}

	template <class Handler, class... Args>
	void runQuery(Args&&... args, Handler&& handler)
	{
		auto q = std::make_shared<Query>(query());
		insert(q, std::forward<Args>(args)...);
		q.run(handler);
	}

	Database(Database&) = delete;
	void operator=(Database&) = delete;

private:
	/// unique_ptr with default deleter for internal use
	typedef std::unique_ptr<Connection> InternalConnectionPtr;

	friend struct OnDeleteReturnToPool;

	ConnectionPtr makeConnectionPtr(Connection* conn)
	{
		return {conn, shared_from_this()};
	}

	/// Used by ConnectionPtr deleter \see OnDeleteReturnToPool
	void returnToPool(Connection* conn)
	{
		strand.dispatch([=]{
			if(requestQueue.empty()){
				connectionPool.emplace_back(conn);
			} else {
				auto handler = std::move(requestQueue.front());
				requestQueue.pop_front();
				handler(makeConnectionPtr(conn));
			}
		});
	}

	/// We use deque in order to avoid being disconnected by the sql server because of timeout.}
	/// Requested connections are front pop'd, returned connections are pushed back'd.
	std::deque<InternalConnectionPtr> connectionPool;

	/// FIFO for awaiting connection requests
	std::deque<std::function<void(ConnectionPtr)>> requestQueue;

	ConnectionInfo connInfo;
	boost::asio::ip::tcp::endpoint endpoint;
	boost::asio::strand strand;
	boost::asio::io_service& ioService;
};


class Connection{
public:
	Connection(boost::asio::io_service& ioService, const ConnectionInfo& connInfo);

	~Connection();

	template <class Handler>
	void connect(const ConnectionInfo& info, Handler&& handler)
	{
		workIoService.post([this]{
			mysql_options(handle, MYSQL_OPT_RECONNECT, 1);

			auto e = mysql_real_connect(
				handle,
				info.endpoint.address().to_string().c_str(),
				info.user.c_str(),
				info.password.c_str(),
				info.schema.c_str(),
				info.endpoint.port(),
				0,
				info.flags);

			callHandler(ioService, std::forward<Handler>(handler), e);
		});
	}

	template <class Handler>
	void runInThread(Handler&& handler)
	{
		workIoService.post(std::forward<Handler>(handler));
	}

private:
	void threadFunc();

	MYSQL handle;
	boost::asio::io_service workIoService;
	boost::asio::io_service::work dummyWork;
	boost::thread thread;
	boost::asio::io_service& ioService;
};


template <class T, int MySqlT>
struct ValueHolder{
	enum{ MySqlT = MySqlT };

	T value;

	template <class U>
	void set(U&& v)
	{
		value.operator=(std::forward<U>(v));
	}

	void* buffer()
	{
		return BufferPtr<T>::get(value);
	}
};

template <class T>
struct BufferPtr{
	static void* get(T&& value)
	{
		return &value;
	}
};

template <>
struct BufferPtr<std::string>{
	static void* get(std::string& str)
	{
		return str.data();
	}
};


// MySQL types follow
struct Null{};

template <class T, int MySqlT>
struct IntegerHolder : public ValueHolder<T, MySqlT>{};

typedef IntegerHolder<char, MYSQL_TYPE_TINY> TinyInt;
typedef IntegerHolder<unsigned char, MYSQL_TYPE_TINY> UTinyInt;
typedef IntegerHolder<short, MYSQL_TYPE_SHORT> SmallInt;
typedef IntegerHolder<unsigned short, MYSQL_TYPE_SHORT> USmallInt;
typedef IntegerHolder<int, MYSQL_TYPE_LONG> Int;
typedef IntegerHolder<unsigned int, MYSQL_TYPE_LONG> Uint;
typedef IntegerHolder<long long, MYSQL_TYPE_LONGLONG> BigInt;
typedef IntegerHolder<unsigned long long, MYSQL_TYPE_LONGLONG> UBigInt;

template <class T, int MySqlT>
struct FloatingPointHolder : public ValueHolder<T, MySqlT>{};

struct Float : public FloatingPointHolder<float, MYSQL_TYPE_FLOAT>{};
struct Double : public FloatingPointHolder<double, MYSQL_TYPE_DOUBLE>{};

template <int MySqlT>
struct StringHolder : public ValueHolder<std::string, MySqlT>{};

typedef StringHolder<MYSQL_TYPE_STRING> String;
typedef StringHolder<MYSQL_TYPE_BLOB> Blob;


template <class Holder, class Derived>
struct ParamFillerBase{
	template <class Value>
	static void fill(MYSQL_BIND& bind, Holder& holder, Value&& value)
	{
		holder.set(std::forward<T>(value));
		bind.buffer_type = Holder::MySqlT;
		bind.buffer = holder.buffer();
		bind.is_null = 0;
		static_cast<Derived*>(this)->fill(bind, holder);
	}

	static void fill(MYSQL_BIND& bind, Holder& holder, const Null& v)
	{
		bind.buffer_tpe = MYSQL_TYPE_NULL;
	}
};

template <class T> struct ParamFiller;

template <class IntT, int MySqlT>
struct ParamFiller<IntegerHolder<IntT, MySqlT>> :
	public ParamFillerBase<IntegerHolder<IntT, MySqlT>, ParamFiller<IntegerHolder<IntT, MySqlT>>>
{
	static void fill(MYSQL_BIND& bind, IntegerHolder<IntT, MySqlT>& holder)
	{
		bind.is_unsigned = std::is_unsigned<IntT>::value;
	}
};

template <int MySqlT>
struct ParamFiller<StringHolder<MySqlT>> :
	public ParamFillerBase<StringHolder<MySqlT>, ParamFiller<StringHolder<MySqlT>>>
{
	static void fill(MYSQL_BIND& bind, StringHolder<MySqlT>& holder)
	{
		bind.buffer_length = holder.value.capacity();
		bind.length = holder.value.length();
	}
};


class ResultSet{
public:

private:
};


template <class... Params>
class Query : public std::enable_shared_from_this<Query<Params...>>{
public:
	typedef std::tuple<Params...> ValueHolders;

	using std::enable_shared_from_this<Query>::shared_from_this;

	template <class... Args>
	Query(Database& db_, std::string stmt, Args&&... args) :
		stmtStr(stmt),
		db(db_)
	{
		checkStatement();
		set<1>(std::forward<Args>(args)...);
	}

	template <class Handler, class... Args>
	Query(Database& db_, std::string stmt, Args&&... args, Handler&& handler) :
		stmtStr(stmt),
		db(db_)
	{
		checkStatement();
		set<1>(std::forward<Args>(args)...);
		execute(std::forward<Handler>(handler));
	}

	Query(Query&&) = default;

	template <int pos, class Arg>
	void set(Arg&& arg)
	{
		static_assert(pos > 0 && pos <= sizeof...(Params), "invalid argument position");

		ParamFiller<decltype(std::get<pos-1>(valueHolders))>::fill(
				params[pos-1], std::get<pos-1>(valueHolders), std::forward<Arg>(arg));
	}

	template <int pos, class Arg, class... Args>
	void set(Arg&& first, Args&&... others)
	{
		insert<pos>(std::forward<Arg>(first));
		insert<pos+1>(std::forward<Args>(others)...);
	}

	template <class Handler>
	void execute(Handler&& handler)
	{
		auto sthis = shared_from_this();
		db.getConnection([sthis, handler](ConnectionPtr&& conn){
			conn.runInThread(std::bind(&Query::executeQuery, sthis, std::move(conn)));
		});
	}

	Query(const Query&) = delete;
	void operator=(Query&) = delete;

private:
	void checkStatement()
	{
		assert(std::count(str.begin(), str.end(), '?') == sizeof...(Params)
			&& "wrong number of parameters in statement");
	}

	// for Query(db, stmt) constructor
	template <int> void set(){}

	template <class Handler>
	void executeQuery(ConnectionPtr& conn)
	{
		auto stmt = mysql_stmt_init(conn->getHandle());

		mysql_stmt_prepare(stmt, stmtStr.data(), stmtStr.length());

		mysql_stmt_bind_param(stmt, params);

		assert(mysql_stmt_param_count(stmt) == sizeof...(Params) && "invalid param count");

		mysql_stmt_execute(stmt);

		ResultSet result(stmt);

		/*auto data = mysql_stmt_result_metadata(stmt);
		int fieldCount = mysql_num_fields(data);
		auto fields = mysql_fetch_fields(data);
		auto params = new MYSQL_BIND[fieldCount];

		for(int i = 0; i < fieldCount; ++i){
			auto field = fields[i];
			field.type
		}*/

		mysql_stmt_bind_result(stmt, result.native);

		mysql_stmt_store_result(stmt);
	}


	MYSQL_BIND params[sizeof...(Params)];
	ValueHolders valueHolders;
	std::string stmtStr;
	Database& db;
};

template <class... Params, class... Args>
inline QueryPtr makeQuery(Args&&... args)
{
	return std::make_shared<Query<Params...>>(std::forward<Args>(args)...);
}

} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_DATABASE_H_
