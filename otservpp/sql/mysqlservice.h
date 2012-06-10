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
#include "../lambdautil.hpp"

namespace otservpp{ namespace sql{ namespace mysql{

struct StmtDeleter;

typedef int Id;
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

	template <class... T>
	using Row = typename mysql::Row<T...>;

	template <class... T>
	using RowSet = typename mysql::RowSet<T...>;

	ConnectionImpl() :
		threadEndFlag((void*)0, noOp)
		//cancelFlag(false)
	{}

	static void noOp(void*){}

	MYSQL handle;
	std::shared_ptr<void> threadEndFlag;
	int connId;
	//std::atomic_bool cancelFlag;
};

/// shared_ptr<mysql_stmt> deleter
struct StmtDeleter{
	void operator()(MYSQL_STMT* stmt)
	{
		::mysql_stmt_close(stmt);
	}
};

/*! A MySQL implementation for sql::BasicConnection
 * Each connection shares a thread pool with all the other connections created by the same
 * Service, at any point in time any connection is using a maximum of one thread from the
 * shared thread pool. For each new connection, a new thread is added to the pool, when a
 * connection is destroyed, "it's" thread is signaled to stop after finishing any other
 * connection task; so there are always the same number of connections and threads. This design
 * is used to simplify message passing through the Service's workIoService.
 *
 * \note All the functions in this class are reentrant
 * \todo Handle loss of connection/reconnection gracefully by assigning new connection ids so
 * 		 everyone (specially the Query classes) that happen to store an id detect the
 * 		 invalidation
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
		workIoService.post(lambdaBind(
		[this, &conn, endpoint, user, password, schema, flags] (Handler&& handler) {
			auto error = !::mysql_real_connect(
					&conn.handle, endpoint.address().to_string().c_str(), user.c_str(),
					password.c_str(), schema.c_str(), endpoint.port(), 0, flags);

			postHandlerOrError(conn, handler, error);
		},
		std::forward<Handler>(handler)
		));
	}

	// TODO implement this
	template <class Handler>
	void executeQuery(ConnectionImpl& conn, const std::string& stmt, Handler&& handler)
	{
		assert("not yet implemented");
		workIoService.post([=, &conn]{

		});
	}

	template <class Handler>
	void prepareQuery(ConnectionImpl& conn, const std::string& str, Handler&& handler)
	{
		workIoService.post(lambdaBind(
		[this, &conn, str] (Handler&& handler) {
			PreparedHandle stmt{::mysql_stmt_init(&conn.handle), StmtDeleter()};
			auto stmtPtr = stmt.get();

			if(stmtPtr){
				int error = ::mysql_stmt_prepare(stmtPtr, str.data(), str.length());
				postHandlerOrError(stmtPtr, handler, error, std::move(stmt));
			} else {
				postError(stmtPtr, handler, std::move(stmt));
			}
		},
		std::forward<Handler>(handler)
		));
	}

	template <class Handler, class... In, class... Out>
	void runPrepared(ConnectionImpl& conn,
			PreparedHandle& stmt,
			const Row<In...>& params,
			RowSet<Out...>& result,
			Handler&& handler)
	{
		postExecStmt(conn, stmt, std::forward<Handler>(handler), &params, &result);
	}

	template <class Handler, class... In, class... Out>
	void runPrepared(ConnectionImpl& conn,
			PreparedHandle& stmt,
			const Row<In...>& params,
			Row<Out...>& result,
			Handler&& handler)
	{
		postExecStmt(conn, stmt, std::forward<Handler>(handler), &params, &result);
	}


	// we won't implement cancellation until needed, its actually difficult to think about
	// proper cancellation points, so its better to make code that doesn't rely on this
	void cancel(ConnectionImpl& conn) = delete;

private:
	/// Initializes the MySQL C library
	/// \note This function is thread-safe
	static void initMySql();

	// stmt dispatching impl functions follow
	template <class Handler, class... Extra>
	void postExecStmt(ConnectionImpl& conn, PreparedHandle& stmt_, Handler&& handler,
			Extra... extra)
	{
		assert(&conn.handle == stmt_->mysql);
		auto stmt = stmt_.get();

		workIoService.post(lambdaBind(
			[this, stmt](Handler&& handler, Extra... extra) {
				startExecStmt(stmt, handler, extra...);
			},
			std::forward<Handler>(handler),
			extra...
		));
	}

	template <class Handler, class... Extra>
	void startExecStmt(MYSQL_STMT* stmt, Handler&& handler, Extra... extra)
	{
		bindStmtInput(stmt, handler, extra...);
	}

	template <class Handler, class Input, class Out>
	void startExecStmt(MYSQL_STMT* stmt, Handler& handler, Input* in, Out* out)
	{
		if(validateStmtParamCount(stmt, out))
			bindStmtInput(stmt, handler, in, out);
		else
			postError(Error::BadFieldCount, handler);
	}

	template <class Handler, class... In, class... Extra>
	void bindStmtInput(MYSQL_STMT* stmt, Handler& handler, const Row<In...>* in, Extra... extra)
	{
		BindInHelper<Row<In...>> bindIn{in};
		if(bindAndExecStmt(bindIn.get(), stmt))
			bindStmtOutput(stmt, handler, extra...);
		else
			postError(stmt, handler);
	}

	template <class Handler, class... In>
	void bindStmtInput(MYSQL_STMT* stmt, Handler& handler, const RowSet<In...>* in)
	{
		BindInHelper<RowSet<In...>> bindIn{in};
		std::vector<uint64_t> affectedRows;

		for(auto& row : *in){
			if(bindAndExecStmt(bindIn.get(), stmt))
				affectedRows.push_back(mysql_stmt_affected_rows(stmt));
			else
				return postError(stmt, handler, std::move(affectedRows));
		}

		postHandler(handler, std::move(affectedRows));
	}

	template <class Handler>
	void bindStmtOutput(MYSQL_STMT* stmt, Handler& handler)
	{
		postHandler(handler, mysql_stmt_affected_rows(stmt));
	}

	template <class Handler, class... Out>
	void bindStmtOutput(MYSQL_STMT* stmt, Handler& handler, Row<Out...>* out)
	{
		BindOutHelper<Row<Out...>> bindOut{out};

		if(::mysql_stmt_bind_result(stmt, bindOut.get()))
			return postError(stmt, handler);

		int status = ::mysql_stmt_fetch(stmt);

		if(status == 0 || status == MYSQL_DATA_TRUNCATED)
			postHandlerOrError(stmt, handler, !bindOut.store(stmt), 1ULL);
		else
			postHandlerOrError(stmt, handler, status == 1);
	}

	template <class Handler, class... Out>
	void bindStmtOutput(MYSQL_STMT* stmt, Handler& handler, RowSet<Out...>* out)
	{
		BindOutHelper<RowSet<Out...>> bindOut{out};

		if(::mysql_stmt_bind_result(stmt, bindOut.get()) || ::mysql_stmt_store_result(stmt))
			return postError(stmt, handler);

		auto numRows = mysql_stmt_num_rows(stmt);
		if(out->size()+numRows > out->max_size())
			return postError(Error::ResultIsTooBig, handler);

		try{
			out->reserve(out->size() + numRows);
		}catch(std::bad_alloc& e){
			mysql_stmt_free_result(stmt);
			return postError(Error::OutOfMemory, handler);
		}

		int status;
		while((status = ::mysql_stmt_fetch(stmt)) == 0 || status == MYSQL_DATA_TRUNCATED){
			if(!bindOut.store(stmt)){
				mysql_stmt_free_result(stmt);
				return postError(stmt, handler);
			}
		}

		mysql_stmt_free_result(stmt);
		postHandlerOrError(stmt, handler, status == 1);
	}

	// stmt helper functions follow
	template <class... Out>
	bool validateStmtParamCount(MYSQL_STMT* stmt, Row<Out...>*)
	{
		return mysql_stmt_field_count(stmt) == sizeof...(Out);
	}

	template <class... Out>
	bool validateStmtParamCount(MYSQL_STMT* stmt, RowSet<Out...>*)
	{
		return mysql_stmt_field_count(stmt) == sizeof...(Out);
	}

	bool bindAndExecStmt(MYSQL_BIND* bindIn, MYSQL_STMT* stmt)
	{
		return !(::mysql_stmt_bind_param(stmt, bindIn) || ::mysql_stmt_execute(stmt));
	}

	// bunch of helpers for dispatching errors/handlers follow
	template <class Handler>
	typename std::enable_if<
		std::is_convertible<uint64_t,
			 typename std::decay<typename function_traits<Handler>::arg2_type>::type>::value,
	void>::type
	doPostHandler(Handler& handler, boost::system::error_code&& e,
			uint64_t rows = 0ULL)
	{
		get_io_service().post(lambdaBind(
				std::move(handler), std::move(e), rows));
	}

	template <class Handler>
	typename std::enable_if<
		std::is_convertible<std::vector<uint64_t>,
			 typename std::decay<typename function_traits<Handler>::arg2_type>::type>::value,
	void>::type
	doPostHandler(Handler& handler, boost::system::error_code&& e,
			std::vector<uint64_t> rows = std::vector<uint64_t>())
	{
		get_io_service().post(lambdaBind(
				std::move(handler), std::move(e), std::move(rows)));
	}

	template <class Handler>
	typename std::enable_if<function_traits<Handler>::arity == 1, void>::type
	doPostHandler(Handler& handler, boost::system::error_code& e)
	{
		get_io_service().post(lambdaBind(
				std::move(handler), std::move(e)));
	}

	template <class Handler, class... Args>
	void doPostHandler(Handler& handler, Args&&... args)
	{
		get_io_service().post(lambdaBind(
						std::move(handler), std::forward<Args>(args)...));;
	}

	template <class Handler, class... Param>
	void postHandler(Handler& handler, Param&&... param)
	{
		doPostHandler(handler, boost::system::error_code(), std::forward<Param>(param)...);
	}

	template <class Handler, class Error, class... Args>
	void doPostError(Handler& handler, Error e, Args&&... args)
	{
		doPostHandler(handler,
				boost::system::error_code(static_cast<int>(e), getErrorCategory()),
				std::forward<Args>(args)...);
	}

	template <class Handler, class... Args>
	void postError(ConnectionImpl& conn, Handler& handler, Args&&... args)
	{
		doPostError(handler, ::mysql_errno(&conn.handle), std::forward<Args>(args)...);
	}

	template <class Handler, class... Args>
	void postError(MYSQL_STMT* stmt, Handler& handler, Args&&... args)
	{
		doPostError(handler, ::mysql_stmt_errno(stmt), std::forward<Args>(args)...);
	}

	template <class Handler, class... Args>
	void postError(Error e, Handler& handler, Args&&... args)
	{
		doPostError(handler, e, std::forward<Args>(args)...);
	}

	template <class Source, class Handler, class Error, class... Args>
	void postHandlerOrError(Source& src, Handler& handler, Error e, Args&&... args)
	{
		if(e)
			postError(src, handler, std::forward<Args>(args)...);
		else
			postHandler(handler, std::forward<Args>(args)...);
	}

	boost::asio::io_service workIoService;
	boost::thread_group threadPool;
	boost::asio::io_service::work dummyWork;
	std::atomic_int connIdFactory;
};

} /* namespace mysql */
} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_MYSQLSERVICE_H_
