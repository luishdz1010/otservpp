#ifndef OTSERVPP_SQL_QUERY_H_
#define OTSERVPP_SQL_QUERY_H_

#include <map>
#include <boost/asio/strand.hpp>
#include "sql.hpp"
#include "connectionpool.h"

namespace otservpp{ namespace sql{

/*! Thread-safe class for executing multiple simple unparameterized queries with the same object
 *
 */
#if 0
class SimpleQuery{
public:
	/*SimpleQuery(ConnectionPool& connPool) :
		connPool(pool)
	{}*/

	template <class String>
	SimpleQuery(ConnectionPool& connPool, String&& str) :
		stmt(std::forward<String>(str)),
		connPool(pool)
	{}

	/*template <class String>
	void reset(String&& stmt_)
	{
		boost::lock_guard<boost::mutex> lock(mutex);
		stmt = std::forward<String>(stmt_);
	}*/

	template <class Handler>
	void execute(Handler& handler)
	{
		connPool.getConnection([=](ConnectionPtr&& conn){
			//boost::shared_lock<boost::shared_mutex> lock(mutex);
			conn->executeQuery(stmt, [handler, conn]
			(const boost::system::error_code& e, ResultSet& res){
				handler(e, res);
			});
		});
	}

private:
	//boost::shared_mutex mutex;
	const std::string stmt;
	ConnectionPool& connPool;
};
#endif

/*! Thread-safe class for executing multiple prepared queries with the same object
 * Since prepared statements are universally bounded to a connection in most DB APIs, we need a
 * way to logically store one prepared statement for all possible connections a ConnectionPool
 * could give us. Since there's no API that let us do that, here we simply create a different
 * prepared statement for each new Connection that is given to us. This adds memory overhead but
 * allow us to concurrently use the "same" prepared statement (from the point of view of the
 * client of this class) up to the maximum number of available connection. Although its arguably
 * whether or not this is a common need, this "caching" mechanism is needed anyway, its not like
 * there are tons of prepared statement declared all over the application, so the generated
 * overhead should be negligible.
 *
 * We use a std::map for the above purpose, but thats just for laziness, later it will probably
 * be changed to a simple array without a need for locking since we will always access different
 * indices. An issue with this though, is that we need to have a fixed connection size, so the
 * pool can't change its connection count without us going to hell.
 */
class PreparedQuery{
public:
	template <class... Values>
	using TuplePtr = std::shared_ptr<std::tuple<Values...>>;

	template <class String>
	PreparedQuery(ConnectionPool& pool, String&& stmt) :
		stmtStr(stmt),
		connPool(pool)
	{}

	/*! Asynchronously executes the prepared query using the given values in the given order
	 * Execute expects a list of parameters equal to the number of parameter markers in the query
	 * or a std::shared_ptr<std::tuple<...>> containing the parameters. The first parameter (or
	 * the first element in the tuple) correspond to the first "?" in the query, and so on.
	 *
	 * After the query completes (or fails) the given handler is called with an error code (if
	 * any) and a result set (an empty set if !!e). This function has to borrow a Connection,
	 * prepare the statement if it hasn't already been prepared and then execute it, many errors
	 * can come up during this process, execute() stops at the first error.
	 *
	 * \note This function is thread-safe when used like execute(val1, val2, ...) and it is also
	 * thread-safe in the execute(shared_ptr<tuple<..>>) version provided nobody else is writing
	 * to the tuple's elements until the handler is dispatched.
	 */
	template <class Handler, class... Values>
	void execute(Values&&... values, Handler&& handler)
	{
		auto control = std::make_shared<ExecuteControl<Handler, Values...>>(
				std::forward<Handler>(handler), std::forward<Values>(values)...);

		connPool.getConnection([this, control](ConnectionPtr&& conn) mutable{
			mutex.lock_shared();
			auto stmt = stmtMap.find(conn->getId());

			if(stmt != stmtMap.end()){
				mutex.unlock_shared();

				executePrepared(*stmt, control, conn);

			} else {
				mutex.unlock_shared();

				conn->prepareQuery(stmtStr, [this, control, conn]
				(const boost::system::error_code& e, Connection::PreparedHandle stmt) mutable{
					if(e){
						control->handler(e, ResultSet::emptySet());
					} else {
						mutex.lock();
						stmtMap.insert(std::make_pair(conn->getId(), stmt));
						mutex.unlock();

						executePrepared(stmt, control, conn);
					}
				});
			}
		});
	}


private:
	template <class Handler, class... Elements>
	struct ExecuteControl{
		template <class H, class... E>
		ExecuteControl(H&& h, E&&... args) :
			values(std::forward<E>(args)...),
			handler(std::forward<H>(h))
		{}

		const std::tuple<Elements...>* getValues() const
		{
			return &values;
		}

		std::tuple<Elements...> values;
		Handler handler;
	};

	template <class Handler, class... Elements>
	struct ExecuteControl<Handler, TuplePtr<Elements...>>{
		template <class H, class V>
		ExecuteControl(H&& h, V&& tuple) :
			handler(std::forward<H>(h)),
			values(std::forward<V>(tuple))
		{}

		const std::tuple<Elements...>* getValues() const
		{
			return values.get();
		}

		Handler handler;
		TuplePtr<Elements...> values;
	};

	template <class Control>
	void executePrepared(Connection::PreparedHandle& stmt, Control& control, ConnectionPtr& conn)
	{
		/*conn->runPrepared(stmt, control.getValues(), [control, conn]
		(const boost::system::error_code& e, ResultSet& res){
			control->handler(e, res);
			// connection dies here and is returned to the pool
		});*/
	}

	boost::shared_mutex mutex;
	std::map<Connection::Id, Connection::PreparedHandle> stmtMap;
	const std::string stmtStr;
	ConnectionPool& connPool;
};


class BatchQuery{

};

} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_QUERY_H_
