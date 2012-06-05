#include "mysqlservice.h"
#include <glog/logging.h>

namespace otservpp{ namespace sql{ namespace mysql{

Service::Service(boost::asio::io_service& ioService) :
	boost::asio::io_service::service(ioService),
	dummyWork(workIoService)
{
	initMySql();
}

void Service::initMySql()
{
	static struct Lib{
		Lib() { throwIfError(::mysql_library_init(0, NULL, NULL)); }
		~Lib(){ ::mysql_library_end(); }
	} init;
}

void Service::shutdown_service()
{
	workIoService.stop();
	threadPool.join_all();
}

ConnectionImpl::Id Service::getId(ConnectionImpl& conn)
{
	return &conn.handle;
}

void Service::create(ConnectionImpl& conn)
{
	if(!mysql_init(&conn.handle))
		throw std::bad_alloc();

	auto thread = std::make_uniqe<boost::thread>();
	auto threadRawPtr = thread.get();
	auto& terminated = conn.threadEndFlag;

	*thread = boost::thread([this, terminated, threadRawPtr]{
		try{
			do
				if(workIoService.run_one() == 0) break;
			while(!*terminated);

			if(!workIoService.stopped())
				threadPool.remove_thread(threadRawPtr);

			mysql_thread_end();

		} catch(std::exception& e){
			LOG(FATAL) << "unexpected exception during a mysql connection operation. "
					"What: " << e.what() << std::endl;
		}
	});

	threadPool.add_thread(thread.release());
}

void Service::destroy(ConnectionImpl& conn)
{
	*conn.threadEndFlag = true;
	mysql_close(&conn.handle);
}

/* TODO delete this if it turns out to be not needed
void Service::cancel(ConnectionImpl& conn)
{
	conn.cancelFlag.store(true, std::memory_order_release);
}*/

} /* namespace mysql */
} /* namespace sql */
} /* namespace otservpp */
