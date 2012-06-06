#ifndef OTSERVPP_SQL_MYSQLSERVICE_H_
#define OTSERVPP_SQL_MYSQLSERVICE_H_

#include <memory>
#include <atomic>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/thread/thread.hpp>
#include <mysql/mysql.h>
#include "error.hpp"
#include "mysqltypes.hpp"

namespace otservpp{ namespace sql{ namespace mysql{

struct StmtDeleter;

typedef MYSQL* Id;
typedef MYSQL* Handle;
typedef std::shared_ptr<MYSQL_STMT> PreparedHandle;

/*! A MySQL ResultSet implementation
 *
 */
class ResultSet{
public:
	ResultSet(){}

	static const ResultSet& emptySet()
	{
		static ResultSet empty;
		return empty;
	}
};

/// The implementation details of the MySQL connection
struct ConnectionImpl{
	typedef mysql::Id Id;
	typedef mysql::Handle Handle;
	typedef mysql::PreparedHandle PreparedHandle;
	typedef mysql::ResultSet ResultSet;

	ConnectionImpl() :
		threadEndFlag(std::make_shared<bool>(false)),
		cancelFlag(false)
	{}

	MYSQL handle;
	// no need for volatile or atomic since we don't need deterministic thread destruction
	std::shared_ptr<bool> threadEndFlag;
	std::atomic_bool cancelFlag;
};


/*! A MySQL implementation for sql::BasicConnection
 * Each connection shares a thread pool with all the other connections created by the same
 * Service, at any point in time any connection is using a maximum of one thread from the
 * shared thread pool. For each new connection, a new thread is added to the pool, when a
 * connection is destroyed, "it's" thread is signaled to stop after finishing any other
 * connection task; so there are always the same number of connections and threads. This design
 * is used to simplify message passing through Service's workIoService.
 *
 * \note All the functions in this class are reentrant
 */
class Service : public boost::asio::io_service::service{
public:
	/// The id of the service as required by io_service::service
	static boost::asio::io_service::id id;

	/// Required by asio::basic_io_object
	typedef ConnectionImpl implementation_type;

	explicit Service(boost::asio::io_service& ioService);

	void shutdown_service() override;

	void construct(ConnectionImpl& conn);
	void destroy(ConnectionImpl& conn);

	Id getId(ConnectionImpl& conn);

	template <class Handler>
	void connect(ConnectionImpl& conn,
			const boost::asio::ip::tcp::endpoint& endpoint,
			const std::string& user,
			const std::string& password,
			const std::string& schema,
			unsigned long flags,
			Handler&& handler)
	{
		workIoService.post([=, &conn]{
			postHandlerOrError(conn, std::forward<Handler>(const_cast<Handler&>(handler)),
				// returns NULL on failure
				!::mysql_real_connect(&conn.handle,
					endpoint.address().to_string().c_str(),
					user.c_str(),
					password.c_str(),
					schema.c_str(),
					endpoint.port(),
					0,
					flags));
		});
	}

	// TODO implement this
	template <class Handler>
	void executeQuery(ConnectionImpl& conn, const std::string& stmt, Handler&& handler)
	{
		assert("not yet implemented");
		workIoService.post([=, &conn]{
			if(isCanceled(conn))
				abortOperation(handler, ResultSet::emptySet());

			//int error = ::mysql_real_query(&conn.handle, stmt.data(), stmt.length());



			/*postHandlerOrError(conn, handler,
				::mysql_real_query(&conn.handle, stmt.data(), stmt.length()));*/
		});
	}

	template <class Handler>
	void prepareQuery(ConnectionImpl& conn, const std::string& stmtStr, Handler&& handler)
	{
		workIoService.post([=, &conn]{
			PreparedHandle stmt{::mysql_stmt_init(&conn.handle), StmtDeleter()};

			if(!stmt)
				return postErrorHandler(stmt, handler, stmt);

			postHandlerOrError(stmt, handler, stmt,
				::mysql_stmt_prepare(stmt.get(), stmtStr.data(), stmtStr.length()));
		});
	}

	template <class Handler, class... Values>
	void executePrepared(ConnectionImpl& conn,
			PreparedHandle& stmt,
			const std::tuple<Values...>* values,
			Handler&& handler)
	{
		workIoService.post([=, &conn]{
			assert(mysql_stmt_param_count(stmt.get()) == sizeof...(Values) &&
					"invalid param count");

			MYSQL_BIND params[sizeof...(Values)] = {}; // zero fill

			fillParams(params, values);

			// both functions return != 0 on failure
			if(::mysql_stmt_bind_param(stmt.get(), params) || ::mysql_stmt_execute(stmt.get()))
				return postErrorHandler(stmt, handler, ResultSet::emptySet());

			std::cout << "Rows: " << mysql_stmt_affected_rows(stmt.get());
		});
	}

	// we won't implement cancellation until needed, its actually difficult to think about
	// proper cancellation points, so its better to make code that doesn't rely on this
	void cancel(ConnectionImpl& conn) = delete;

private:
	/// Initializes the MySQL C library
	/// \note This function is thread-safe
	static void initMySql();

	// Bunch of helper functions for dispatching handlers follow...
	bool isCanceled(ConnectionImpl& conn)
	{
		return conn.cancelFlag.load(std::memory_order_acquire);
	}

	template <class Handler, class... Args>
	void abortOperation(Handler&& handler, Args&&... args)
	{
		postHandler(std::forward<Handler>(handler), boost::asio::error::operation_aborted,
				std::forward<Args>(args)...);
	}

	template <class Handler, class... Args>
	void postHandler(Handler&& handler, Args&&... args)
	{
		get_io_service().post(makeLambda(std::forward<Handler>(handler),
				std::forward<Args>(args)...));
	}

	template <class Source, class Handler, class Error, class... Args>
	void postHandlerOrError(Source& src, Handler&& handler,  Args&&... args, Error e)
	{
		if(e)
			postErrorHandler(src, std::forward<Handler>(handler), std::forward<Args>(args)...);
		else
			postHandler(std::forward<Handler>(handler), boost::system::error_code(),
					std::forward<Args>(args)...);
	}

	template <class Handler, class... Args>
	void postErrorHandler(ConnectionImpl& conn, Handler&& handler, Args&&... args)
	{
		doPostErrorHandler(std::forward<Handler>(handler),
				::mysql_errno(&conn.handle), std::forward<Args>(args)...);
	}

	template <class Handler, class... Args>
	void postErrorHandler(PreparedHandle& stmt, Handler&& handler, Args&&... args)
	{
		doPostErrorHandler(std::forward<Handler>(handler),
				::mysql_stmt_errno(stmt.get()), std::forward<Args>(args)...);
	}

	template <class Handler, class... Args>
	void doPostErrorHandler(Handler&& handler, uint e, Args&&... args)
	{
		postHandler(std::forward<Handler>(handler),
				boost::system::error_code(static_cast<int>(e), getErrorCategory()),
				std::forward<Args>(args)...);
	}

	boost::asio::io_service workIoService;
	boost::thread_group threadPool;
	boost::asio::io_service::work dummyWork;
};


/// shared_ptr<mysql_stmt> deleter
struct StmtDeleter{
	void operator()(MYSQL_STMT* stmt)
	{
		::mysql_stmt_close(stmt);
	}
};


} /* namespace mysql */
} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_MYSQLSERVICE_H_
