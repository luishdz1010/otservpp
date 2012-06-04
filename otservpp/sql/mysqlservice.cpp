#include "mysqlservice.h"
#include <glog/logging.h>

namespace otservpp { namespace sql{

MySqlService::MySqlService(boost::asio::io_service& ioService) :
	boost::asio::io_service::service(ioService),
	dummyWork(workIoService)
{
	initMySql();
}

void MySqlService::initMySql()
{
	static struct Lib{
		Lib() { throwIfError(::mysql_library_init(0, NULL, NULL)); }
		~Lib(){ ::mysql_library_end(); }
	} init;
}

void MySqlService::shutdown_service()
{
	workIoService.stop();
	threadPool.join_all();
}

void MySqlService::create(ConnectionImpl& conn)
{
	if(!mysql_init(&conn.handle))
		throw std::bad_alloc();

	auto thread = std::make_uniqe<boost::thread>();
	auto threadRawPtr = thread.get();
	auto& terminated = conn.threadEndFlag;

	*thread = boost::thread([this, terminated, threadRawPtr]{
		try{
			while(!*terminated)
				workIoService.run_one();

			threadPool.remove_thread(threadRawPtr);
		} catch(std::exception& e){
			LOG(FATAL) << "unexpected exception during a mysql connection operation. "
					"What: " << e.what() << std::endl;
		}
	});

	threadPool.add_thread(thread.release());
}

void MySqlService::destroy(ConnectionImpl& conn)
{
	*conn.threadEndFlag = true;
	mysql_close(&conn.handle);
}

void MySqlService::cancel(ConnectionImpl& conn)
{
	conn.cancelFlag = false;
}

} /* namespace sql */
} /* namespace otservpp */
