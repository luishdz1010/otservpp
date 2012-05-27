#include <iostream>
//#include <boost/locale.hpp>
#include <glog/logging.h>
#include "connection.hpp"
#include "protocol/loginprotocol.h"
#include "scheduler.hpp"
#include "script/adapterset.hpp"
#include "script/hook/loginprotocolhooks.h"

extern "C"{
#include "lua5.1/lualib.h"
}
//using namespace boost::locale;
using namespace otservpp;

int main(int argc, char* argv[]) {
	google::InitGoogleLogging(argv[0]);
	script::AccountLoginSuccedKeys gool;

	luabind::object o;

	lua_State* s = luaL_newstate();
	luaL_openlibs(s);
	luabind::open(s);
	luaL_dostring(s,
		"function hook(a, b)"
		"   print(a, b)   "
		"   return {motd='MOTD', characters={{}}, premiumEnd=0}"
		"end"
	);

	auto shook = script::AccountLoginSucced();
	shook.hookUp(luabind::globals(s)["hook"]);

	hook::AccountLoginSucced hook = shook;

	//std::cout << std::get<0>(hook(12, "Lol")) << std::endl;


	hook(10, "hello");
	std::cout << "dumb";
	/*generator gen;
	std::locale l = gen("");
	std::locale::global(l);
	std::cout.imbue(l);



	std::cout << as::date << std::time(0) << std::endl;*/


	//boost::asio::io_service service;
	//boost::asio::ip::tcp::socket s(service);
	/*
	auto css = std::make_shared<otservpp::Connection<otservpp::LoginProtocol>>(boost::asio::ip::tcp::socket(service), otservpp::LoginProtocol());
	//otservpp::Connection<otservpp::LoginProtocol> css {boost::asio::ip::tcp::socket(service)};
	css->start();*/

	return 0;
}
