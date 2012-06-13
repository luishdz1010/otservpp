#ifndef OTSERVPP_SQL_CONNECTIONPOOL_H_
#define OTSERVPP_SQL_CONNECTIONPOOL_H_

#include <deque>
#include <algorithm>
#include <boost/asio/strand.hpp>
#include "sqldcl.hpp"

namespace otservpp{ namespace sql{

/*! The ConnectionPool class manages a set of resuable Connection objects
 * Each ConnectionPool object has a fixed set of Connection objects that is set during
 * construction. Whenever an client object needs a Connection, it is requested by passing
 * a handler to getConnection(), if theres a connection available at that moment, the request
 * is served by passing a ConnectionPtr to the handler, otherwise the request is queued until
 * a Connection is made available.
 *
 * After using a Connection, it must be returned to the pool, this is done automatically by
 * the destructor of the passed ConnectionPtr. Clients must not store the passed connection.
 *
 * This class is not copyable or movable, this has to do with the mechanism whereby the requested
 * Connections are returned to the pool.
 *
 * All the clients of the pool must return any given Connection to the pool before it is
 * destroyed.
 */
class ConnectionPool{
public:
	/// Creates a new ConnectionPool with connectionNumber connections in it.
	ConnectionPool(boost::asio::io_service& ioService, uint connectionNumber);

	/// Returns the io_service associated with the ConnectionPool
	boost::asio::io_service& getIoService()
	{
		return strand.get_io_service();
	}

	/*! Starts all the pool's connections using the given parameters.
	 * This function returns immediately.
	 *
	 * After attempting to start every connection, the given handler is called with the very
	 * first error occurred between all connection attempts (if any) and the number of successful
	 * started connections. The signature of the handler function must be:
	 *
	 * \code void handler(const boost::system::error_code& e, uint startedConnections) \endcode
	 *
	 * If !!e == false then startedConnections == connectionNumber (given in constructor).
	 *
	 * The ConnectionPool object must be alive at least until the handler is called.
	 *
	 * \note Although this function is thread-safe, it must only be called once on each
	 * 		 ConnectionPool object
	 */
	template <class Handler>
	void connect(const boost::asio::ip::tcp::endpoint& endpoint,
					  const std::string& user,
					  const std::string& password,
					  const std::string& schema,
					  uint flags,
					  Handler&& handler)
	{
		strand.dispatch([=]{
			auto control = std::make_shared<ConnectControl>(std::forward<Handler>(handler), pool);

			for(auto& conn : pool){
				conn->connect(endpoint, user, password, schema, flags,
				strand.wrap([&conn, control](const boost::system::error_code& e){
					if(e){
						control->pool.erase(
								std::find(control->pool.begin(), control->pool.end(), conn));

						if(!control->firstError)
							control->firstError = e;
					}

					if(control.unique())
						control->handler(control->firstError, control->pool.size());
				}));
			}
		});
	}

	/*! Obtains a ConnectionPtr from this ConnectionPool and passes it to the given handler
	 * The handler might be executed during in the body of this function if:
	 * \li There's at least one connection left in the pool
	 * \li There's no other concurrent operation in this ConnectionPool
	 * If these requirements can't be meet, the given handler is stored and executed as soon as
	 * possible. The handler signature must be:
	 * \code
	 * 	void handler(ConnectionPtr conn)
	 * \endcode
	 * \note This function is thread-safe
	 */
	template <class Handler>
	void getConnection(Handler&& handler)
	{
		strand.dispatch([=]()mutable{
			if(pool.empty()){
				requestQueue.emplace_back(std::forward<Handler>(handler));
			} else {
				// pop before calling for reentrancy & exeception safety
				auto conn = makeConnectionPtr(pool.front().release());
				pool.pop_front();
				handler(std::move(conn));
			}
		});
	}

	ConnectionPool(ConnectionPool&) = delete;
	void operator=(ConnectionPool&) = delete;

private:
	// unique_ptr with default deleter for internal use
	typedef std::unique_ptr<Connection> InternalConnectionPtr;
	typedef std::deque<InternalConnectionPtr> InternalPool;
	typedef std::deque<std::function<void(ConnectionPtr)>> ConnectionRequestQueue;

	// helper for asyncConnect
	template <class Handler>
	struct ConnectControl{
		template <class U>
		ConnectControl(U&& h, InternalPool& p) :
			handler(std::forward<U>(h)),
			pool(p)
		{}

		Handler handler;
		boost::system::error_code firstError;
		InternalPool& pool;
	};

	friend struct OnDeleteReturnToPool;

	ConnectionPtr makeConnectionPtr(Connection* conn);

	/// Used by ConnectionPtr deleter \see OnDeleteReturnToPool
	void returnToPool(Connection* conn);

	/// Requested connections are front pop'd, returned connections are pushed back'd. This
	/// in order to avoid being disconnected by the server because of timeout.
	InternalPool pool;

	/// FIFO for awaiting connection requests
	ConnectionRequestQueue requestQueue;

	boost::asio::strand strand;
};


/*! Delete functor for returning unused connections to the connection pool in Database.
 * Instances of this structure are passed as the Deleter objects to any given ConnectionPtr,
 * whenever a ConnectionPtr goes out of scope, the underlying connection is returned to the
 * Database's connection pool instead of being deleted. NOT TRUE CURRENTLY: If the database has
 * already been deleted by the time the ConnectionPtr goes out of scope, the Connection is
 * deleted.
 * \note This functor is thread-safe
 */
struct OnDeleteReturnToPool{
	ConnectionPool* db;

	void operator()(Connection* conn);
};

} /* namespace otservpp */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_CONNECTIONPOOL_H_
