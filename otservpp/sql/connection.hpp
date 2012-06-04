#ifndef OTSERVPP_SQL_CONNECTION_HPP_
#define OTSERVPP_SQL_CONNECTION_HPP_

#include <boost/asio/basic_io_object.hpp>
#include "mysqlservice.h"

namespace otservpp{ namespace sql{

/*! The BasicConnection class provides asynchronous database connection and query functionality
 * This class forwards its operations an underlying SqlService, at the moment we only support
 * MySQL as the DBMS, but using the asio service schema we can add more later, without too much
 * pain.
 *
 * A BasicConnection object can only have one asynchronous operation running at any given time.
 *
 * This class is only intended to be use as a connector and flag setting mechanism, although it
 * provides query functionality, it's better to use the Query and QueryFactory classes, alongside
 * with a ConnectionPool object.
 */
template <class SqlService>
class BasicConnection : public boost::asio::basic_io_object<SqlService>{
public:
	/// Creates a new connection object that isn't connected to any database
	BasicConnection(boost::asio::io_service& ioService) :
		boost::asio::basic_io_object<SqlService>(ioService)
	{}

	/*! Connects asynchronously to the indicated database
	 * This function always return immediately, when the operation completes or fails
	 * the given handler is called with a error code as its first argument:
	 * \code void handler(const boost::system::error_code& e) \endcode
	 */
	template <class Handler>
	void connect(const boost::asio::ip::tcp::endpoint& endpoint,
					  const std::string& user,
					  const std::string& password,
					  const std::string& schema,
					  uint flags,
					  Handler&& handler)
	{
		get_service()->connect(get_implementation(),
				endpoint, user, password, schema, flags, std::forward<Handler>(handler));
	}

	/// Signals the active operation to be canceled as soon as possible
	/// If the operation could be canceled the operation's handler is called with a
	/// boost::asio::operation_aborted error code, otherwise the operation is called normally.
	void cancel()
	{
		get_service()->cancel(get_implementation());
	}

	BasicConnection(BasicConnection&) = delete;
	void operator=(BasicConnection&) = delete;
};

} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_CONNECTION_HPP_
