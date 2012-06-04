#ifndef OTSERVPP_SQL_MYSQLSERVICE_H_
#define OTSERVPP_SQL_MYSQLSERVICE_H_

#include <boost/asio/io_service.hpp>
#include <mysql/mysql.h>
#include "error.hpp"

namespace otservpp { namespace sql{

/*! MySQL implementation for sql::BasicConnection
 * Each connection shares a thread pool with all the other connections created by the same
 * MySqlService, at any point in time any connection is using a maximum of one thread from the
 * shared thread pool. For each new connection, a new thread is added to the pool, when a
 * connection is destroyed, "it's" thread is signaled to stop after finishing any other
 * connection task; so there are always the same number of connections and threads. This design
 * is used to simplify message passing through MySqlService's workIoService.
 */
class MySqlService : public boost::asio::io_service::service{
public:
	struct ConnectionImpl;

	typedef ConnectionImpl implementation_type;

	explicit MySqlService(boost::asio::io_service& ioService);

	void shutdown_service() override;

	void create(ConnectionImpl& conn);
	void destroy(ConnectionImpl& conn);

	template <class Handler>
	void connect(ConnectionImpl& conn,
			const boost::asio::ip::tcp::endpoint& endpoint,
			const std::string& user,
			const std::string& password,
			const std::string& schema,
			unsigned long flags,
			Handler&& handler)
	{
		workIoService.post([=]{
			if(conn.cancelFlag)
				abortOperation(handler);
			else
				dispatchHandler(conn, handler,
					::mysql_real_connect(
						&conn.handle, endpoint.address().to_string().c_str(),
						user.c_str(), password.c_str(), schema.c_str(),
						endpoint.port(), 0, flags));
		});
	}

	void cancel(ConnectionImpl& conn);

private:
	/// Initializes the MySQL C library
	/// \note This function is thread-safe
	static void initMySql();

	template <class Handle>
	void abortOperation(Handler& handler)
	{
		doDispatchHandler(handler, boost::asio::error::operation_aborted);
	}

	template <class Handler, class... Args>
	void doDispatchHandler(Handler& handler, const boost::system::error_code& e, Args&... args)
	{
		get_io_service().post([handler, e, args...]{ handler(e, args...); });
	}

	template <class Handler, class Error, class... Args>
	void dispatchHandler(ConnectionImpl& conn, Handler& handler, Error& e, Args&... args)
	{
		if(conn.cancelFlag)
			abortOperation(handler);
		 else
			doDispatchHandler(handler,
				e? boost::system::error_code() :
				   boost::system::error_code(mysql_errno(conn.handle), getErrorCategory()),
				args...);
	}

	boost::asio::io_service workIoService;
	boost::thread_group threadPool;
	boost::asio::io_service::work dummyWork;
};

/// The implementation details of the MySQL connection
struct MySqlService::ConnectionImpl{
	ConnectionImpl() :
		threadEndFlag(std::make_shared<std::atomic_bool>(false)),
		cancelFlag(false)
	{}

	MYSQL handle;
	std::shared_ptr<std::atomic_bool> threadEndFlag;
	std::atomic_bool cancelFlag;
};

} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_MYSQLSERVICE_H_
