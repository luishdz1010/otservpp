#ifndef OTSERVPP_SQL_QUERY_H_
#define OTSERVPP_SQL_QUERY_H_

#include <map>
#include <boost/asio/strand.hpp>

namespace otservpp{ namespace sql{

/*! Thread-safe class for executing multiple simple unparameterized queries with the same object
 *
 */
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

/*! Thread-safe class for executing multiple prepared queries with the same object
 *
 */
class PreparedQuery{
public:
	template <class String>
	PreparedQuery(ConnectionPool& pool, String&& stmt) :
		stmtStr(stmt),
		connPool(pool)
	{}

	template <class Handler, class... Values>
	void execute(Values&&... values, Handler&& handler)
	{
		auto control = std::make_shared<ExecuteControl<Handler, Values...>>(
				std::forward<Handler>(handler), std::forward<Values>(values)...);

		connPool.getConnection([this, control](ConnectionPtr&& conn){
			mutex.lock_shared();
			auto stmt = stmtMap.find(conn->getId());

			if(stmt != stmtMap.end()){
				mutex.unlock_shared();

				conn->executeQuery(*stmt, control->values, [control, conn]
				(const boost::system::error_code& e, ResultSet& res){
					control->handler(e, res);
				});

			} else {
				mutex.unlock_shared();

				conn->prepareQuery(stmtStr, [this, control, conn]
				(const boost::system::error_code& e, Connection::PreparedHandle stmt){
					if(e){
						control->handler(e, ResultSet::emptySet());
					} else {
						mutex.lock();
						stmtMap.insert(std:: make_pair(conn->getId(), stmt));
						mutex.unlock();

						conn->executeQuery(stmt, control->values, [control, conn]
						(const boost::system::error_code& e, ResultSet& res){
							control->handler(e, res);
						});
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

		std::tuple<Elements...> values;
		Handler handler;
	};

	boost::shared_mutex mutex;
	std::map<Connection::Id, Connection::PreparedHandle> stmtMap;
	const std::string stmtStr;
	ConnectionPool& connPool;
};


class BatchInsertQuery{

};

} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_QUERY_H_
