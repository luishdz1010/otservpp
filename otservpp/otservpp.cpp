#include <iostream>
#include <glog/logging.h>
#include "protocol/accountloginprotocol.h"
#include "scheduler.hpp"
#include "script/adapterset.hpp"
#include "service/servicemanager.h"
#include "sql/sql.hpp"

extern "C"{
#include "lua5.1/lualib.h"
}

using namespace otservpp;
using namespace std;

void work()
{
	boost::asio::io_service svc;
	sql::Connection conn{svc};
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), 3306);
	//amy::auth_info auth("root", "7deabrilde2021");


	conn.connect(endpoint, "root", "7deabrilde2021", "otservpp", 0, [&conn]
	(const boost::system::error_code& e) mutable{
		if(e){
			cout << "error " << e << endl;
			return;
		}

		cout << "connected" << endl;
	});

	svc.run();
}

int main()
{
	work();

	/*conn.async_connect(endpoint, auth, "otservpp", amy::default_flags, [&conn]
    (const boost::system::error_code& e){
		if(e){
			cout << "error " << e << endl;
			return;
		}

		cout << "connected" << endl;
		conn.query("SELECT name, password FROM accounts");
		amy::result_set result = conn.store_result();

		for(const amy::row& row  : result){
			std::cout << row[0].as<amy::sql_tinyint_unsigned>();
		}
	});*/





	return 0;
}


/*struct Protocol;

struct otservpp::StandardOutMessage{
	boost::asio::mutable_buffers_1 getBuffer()
	{
		return boost::asio::buffer(b);
	}

	array<char, 10000> b;
};

struct DumbService : public BasicService<AccountLoginProtocol>{
	DumbService(boost::asio::io_service& io, uint16_t p) :
		BasicService(p),
		svc(io)
	{}

	ProtocolPtr<AccountLoginProtocol> makeProtocol(const ConnectionType& c)
	{
		LoginSharedData s;
		return make_shared<AccountLoginProtocol>(c, s, svc);
	}

	boost::asio::io_service& svc;
};


int main(int argc, char* argv[]) {
	google::InitGoogleLogging(argv[0]);
	google::LogToStderr();

	boost::asio::io_service io;
	ServiceManager svcs(io);

	svcs.registerService(make_uniqe<DumbService>(io, 7171));
	svcs.start();

	io.run();

	return 0;
}*/
