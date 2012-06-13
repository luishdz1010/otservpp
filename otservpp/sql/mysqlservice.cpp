#include "mysqlservice.h"
#include <glog/logging.h>
#include "../forwarddcl.hpp"

namespace otservpp{ namespace sql{ namespace mysql{

boost::asio::io_service::id Service::id;

Service::Service(boost::asio::io_service& ioService) :
	boost::asio::io_service::service(ioService),
	dummyWork(workIoService),
	connIdFactory(1)
{
	initMySql();
}

void Service::initMySql()
{
	static auto init = makeScopedOperation([]{
		if(::mysql_library_init(0, 0, 0))
			throw std::bad_alloc();
	}, []{
		::mysql_library_end();
	});
}

void Service::shutdown_service()
{
	workIoService.stop();
	threadPool.join_all();
}

ConnectionImpl::Id Service::getId(ConnectionImpl& conn)
{
	return conn.connId;
}

void Service::construct(ConnectionImpl& conn)
{
	if(!mysql_init(&conn.handle))
		throw std::bad_alloc();

	conn.connId = connIdFactory.fetch_add(1, std::memory_order_acq_rel);

	auto thread = std::make_uniqe<boost::thread>();
	auto threadRawPtr = thread.get();
	auto& terminated = conn.threadEndFlag;

	*thread = boost::thread([this, terminated, threadRawPtr]{
		try{
			mysql_thread_init();

			do
				if(workIoService.run_one() == 0) break;
			while(!terminated.unique());

			if(!workIoService.stopped())
				threadPool.remove_thread(threadRawPtr);

			mysql_thread_end();

		} catch(std::exception& e){
			LOG(FATAL) << "unexpected exception during a mysql operation. "
					"What: " << e.what();
		}
	});

	threadPool.add_thread(thread.release());
}

void Service::destroy(ConnectionImpl& conn)
{
	mysql_close(&conn.handle);
}

} /* namespace mysql */
} /* namespace sql */
} /* namespace otservpp */
