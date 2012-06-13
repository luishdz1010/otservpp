#ifndef OTSERVPP_SQL_CONNECTION_HPP_
#define OTSERVPP_SQL_CONNECTION_HPP_

#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace otservpp{ namespace sql{

/*! The BasicConnection class provides asynchronous database connection and query functionality
 * This class forwards its operations an underlying SqlService, at the moment we only support
 * MySQL as the DBMS, but using the asio service schema we can add more later, without too much
 * pain.
 *
 * A BasicConnection object can only have one asynchronous operation running at any given time.
 * So this class is not thread safe, but every function inside it is reentrant.
 *
 * This class is only intended to be use as a connector and flag setting mechanism, although it
 * provides query functionality, it's better to use the Query and QueryFactory classes, alongside
 * with a ConnectionPool object.
 */
template <class SqlService>
class BasicConnection : public boost::asio::basic_io_object<SqlService>{
public:
	typedef SqlService Service;
	typedef typename SqlService::implementation_type Impl;
	typedef typename Impl::Id Id;
	typedef typename Impl::Handle Handle;
	typedef typename Impl::PreparedHandle PreparedHandle;
	typedef typename Impl::ResultSet ResultSet;

	template <class... T>
	using Row = typename Impl::template Row<T...>;

	template <class... T>
	using RowSet = typename Impl::template RowSet<T...>;

	/// Creates a new connection object that isn't connected to any database
	BasicConnection(boost::asio::io_service& ioService) :
		boost::asio::basic_io_object<SqlService>(ioService)
	{}

	Id getId()
	{
		return get_service().getId(get_implementation());
	}

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
		get_service().connect(get_implementation(),
				endpoint, user, password, schema, flags, std::forward<Handler>(handler));
	}

	template <class Handler>
	void executeQuery(const std::string& stmt, Handler&& handler)
	{
		get_service().executeQuery(get_implementation(),
				stmt, std::forward<Handler>(handler));
	}

	template <class Handler>
	void prepareQuery(const std::string& stmt, Handler&& handler)
	{
		get_service().prepareQuery(get_implementation(),
				stmt, std::forward<Handler>(handler));
	}

	template <class Handler, class... In, class... Out>
	void runPrepared(PreparedHandle& stmt,
			const std::tuple<In...>& in,
			std::tuple<Out...>& out,
			Handler&& handler)
	{
		get_service().runPrepared(get_implementation(),
				stmt, in, out, std::forward<Handler>(handler));
	}

	template <class Handler, class... In, class... Out>
	void runPrepared(PreparedHandle& stmt,
			const Row<In...>& in,
			RowSet<Out...>& out,
			Handler&& handler)
	{
		get_service().runPrepared(get_implementation(),
				stmt, in, out, std::forward<Handler>(handler));
	}

	/// Signals the active operation to be canceled as soon as possible
	/// If the operation could be canceled the operation's handler is called with a
	/// boost::asio::operation_aborted error code, otherwise the operation is called normally.
	/// \note This is not yet implemented
	void cancel()
	{
		get_service().cancel(get_implementation());
	}

	BasicConnection(BasicConnection&) = delete;
	void operator=(BasicConnection&) = delete;

private:
	using boost::asio::basic_io_object<SqlService>::get_service;
	using boost::asio::basic_io_object<SqlService>::get_implementation;
};

} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_CONNECTION_HPP_
