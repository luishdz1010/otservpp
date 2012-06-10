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

int moved = 0;
	int copied = 0;

void work()
{

	boost::asio::io_service svc;
	boost::asio::io_service::work w(svc);
	sql::Connection conn{svc};
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), 3306);
	//amy::auth_info auth("root", "7deabrilde2021");


	struct t{
		t(){}
		t(const t&){ ++copied; }
		t(t&&) { ++moved; }
	} test;

	auto h = [&, test](const boost::system::error_code& e){
		if(e){
			cout << "error " << e << endl;
			return;
		}

		cout << "connected" << endl;

		conn.prepareQuery("SELECT password FROM accounts WHERE LENGTH(name) > ?", [&]
	    (const boost::system::error_code& e, sql::Connection::PreparedHandle stmt){
			if(e){
				cout << "prep error: " << e << endl;
				return;
			}

			cout << "QUERY PREPARED args: " << mysql_stmt_param_count(stmt.get()) << endl;

			auto in = make_shared<tuple<int>>(3);
			auto out = make_shared<vector<tuple<string>>>();
			conn.runPrepared(stmt, *in, *out, [stmt, in, out]
			(const boost::system::error_code& e){
				if(e){
					cout << "exec error " << e << endl;
					return;
				}

				cout << "QUERY EXECUTED count: " << out->capacity() << "  passwords: " << endl;

				for(auto& pass : *out)
					cout << "  " << std::get<0>(pass) << endl;
			});
		});
	};

	conn.connect(endpoint, "root", "7deabrilde2021", "otservpp", 0, h);

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
