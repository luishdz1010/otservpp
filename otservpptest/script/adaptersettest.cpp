#include <gtest/gtest.h>
#include "otservpp/script/adapterset.hpp"
#include <luabind/luabind.hpp>

using namespace otservpp::script;

namespace{
	bool adapter1Ran;
	bool adapter2Ran;

	struct dummy1{};
	struct TestAdapter1 : public TypeAdapter<dummy1>{
		void apply(lua_State* L)
		{
			adapter1Ran = true;
			ASSERT_TRUE(L);
		}
	};
	OTSERVPP_REG_STATIC_ADAPTER(TestAdapter1);

	struct dummy2{};
	struct TestAdapter2 : public TypeAdapter<dummy2>{
		void apply(lua_State* L)
		{
			adapter2Ran = true;
			ASSERT_TRUE(L);
		}

	};
	OTSERVPP_REG_STATIC_ADAPTER(TestAdapter2);

	class FunctionAdapter : public GlobalInstanceAdapter<void()>{
	public:
		FunctionAdapter(bool& b_) :
			GlobalInstanceAdapter<void()>("testFunc"),
			b(b_)
		{}

		void apply(lua_State* L)
		{
			luabind::module(L)
			[
			 luabind::def("testFunc", [this](){ b = !b; })
			];
		}

	private:
		bool& b;
	};
}

class AdapterSetTest : public ::testing::Test{
protected:
	void SetUp()
	{
		adapter1Ran = false;
		adapter2Ran = false;

		L = luaL_newstate();
		luabind::open(L);
	}

	void TearDown()
	{
		lua_close(L);
	}

	lua_State* L;
};


TEST_F(AdapterSetTest, AppliesTypeStaticAdapters){
	apply(StaticAdapters::getAll(), L);

	ASSERT_TRUE(adapter1Ran);
	ASSERT_TRUE(adapter2Ran);
}

TEST_F(AdapterSetTest, AppliesGlobalInstanceAdapters){
	AdapterSet adapters;
	bool flag = false;
	adapters.insert(std::make_shared<FunctionAdapter>(flag));
	apply(adapters, L);

	luaL_dostring(L, "testFunc()");
	ASSERT_TRUE(flag) << "Lua isn't executing defined functions";
	luaL_dostring(L, "testFunc()");
	ASSERT_FALSE(flag) << "Lua isn't executing functions properly";
}

TEST_F(AdapterSetTest, DoesNotAllowDuplicatedAdapters){
	AdapterSet adapter;
	bool dummy;
	ASSERT_TRUE(adapter.emplace(std::make_shared<FunctionAdapter>(dummy)).second);
	ASSERT_FALSE(adapter.emplace(std::make_shared<FunctionAdapter>(dummy)).second);
}
