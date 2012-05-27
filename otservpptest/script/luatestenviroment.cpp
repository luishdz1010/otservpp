#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <algorithm>
#include "otservpp/script/adapterset.hpp"
#include "otservpp/script/adapter/scheduler.h"
#include <luabind/lua_include.hpp>
#include <luabind/tag_function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

extern "C"{
#include <lualib.h>
}

// To create a new lua test place under /scripts, every file in there will be runned by
// the test enviroment.

using namespace std;
using namespace otservpp::script;

namespace{

int errorHandler(lua_State* L)
{
	lua_Debug d;
   lua_getstack(L, 1, &d);
   lua_getinfo(L, "Sln", &d);
   std::string err = lua_tostring(L, -1);
   lua_pop(L, 1);
   std::stringstream msg;
   msg << d.short_src << ":" << d.currentline;

   if (d.name != 0)
   {
	  msg << "(" << d.namewhat << " " << d.name << ")";
   }
   msg << " " << err;
   lua_pushstring(L, msg.str().c_str());
   return 1;
}
}


struct gtest{};
OTSERVPP_STATIC_ADAPTER_T(TestingFacilities, gtest){
	using namespace luabind;
	/*module(L)
	[
	];*/
}

class LuaTestEnviroment : public ::testing::Test{
protected:

	void SetUp()
	{
		adapters.emplace(new SchedulerAdapter(scheduler));

		L = luaL_newstate();
		luaL_openlibs(L);
		luabind::open(L);
		//luabind::set_pcall_callback(&errorHandler);
		apply(StaticAdapters::getAll(), L);
		apply(adapters, L);
	}

	void TearDown()
	{
		lua_close(L);
	}

	lua_State* L;
	AdapterSet adapters;
	boost::asio::io_service ioService;

	otservpp::Scheduler scheduler {ioService};
};

TEST_F(LuaTestEnviroment, TestAllScripts){
	using namespace boost::filesystem;

	path scriptsDir = initial_path()/"script/scripts";
	ASSERT_TRUE(exists(scriptsDir)) << scriptsDir;
	ASSERT_TRUE(is_directory(scriptsDir));

	vector<path> scripts;

	std::for_each(directory_iterator(scriptsDir), directory_iterator(),
	[&scripts](const path& script){
		if(is_regular_file(script)
			&& script.extension() == ".lua"
			&& script.filename().string().front() != '-')
				scripts.push_back(script);
	});

	sort(scripts.begin(), scripts.end());

	try{
		for(auto& path : scripts){
			if(luaL_loadfile(L, path.c_str()) != 0)
				ADD_FAILURE() << lua_tostring(L, -1);
			if(lua_pcall(L, 0, 0, 0) != 0)
				ADD_FAILURE() << lua_tostring(L, -1);
		}

		ioService.run();
	} catch(luabind::error& e){
		ADD_FAILURE() << e.what() << endl << lua_tostring(L, -1);
	}
}
