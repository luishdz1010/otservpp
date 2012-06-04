#include "connectionpool.h"

namespace otservpp{ namespace sql{

ConnectionPool::ConnectionPool(boost::asio::io_service& ioService, uint connectionNumber) :
	strand(ioService)
{
	for(uint i = 0; i < connectionNumber; ++i)
		pool.emplace_back(new Connection(ioService));
}

ConnectionPtr ConnectionPool::makeConnectionPtr(Connection* conn)
{
	return {conn, OnDeleteReturnToPool{this}};
}

void ConnectionPool::returnToPool(Connection* conn)
{
	strand.dispatch([=]{
		if(requestQueue.empty()){
			connectionPool.emplace_back(conn);
		} else {
			// pop before calling for reentrancy & exeception safety
			auto handler = std::move(requestQueue.front());
			requestQueue.pop_front();
			handler(makeConnectionPtr(conn));
		}
	});
}

void OnDeleteReturnToPool::operator()(Connection* conn)
{
	//if(auto db = wdb.lock())
		db->returnToPool(conn);
	//else
		//delete conn;
}

} /* namespace sql */
} /* namespace otservpp */
