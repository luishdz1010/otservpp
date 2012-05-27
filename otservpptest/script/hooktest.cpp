#include <gtest/gtest.h>
#include "otservpp/hook/hook.hpp"
#include "otservpp/script/hook/hookutil.hpp"

extern "C"{
#include "lualib.h"
}

using namespace std;
using namespace otservpp;

namespace{

typedef hook::Hook<string(int, int)> SimpleHook;

typedef tuple<string, int, luabind::object> TupleResult;
typedef hook::Hook<TupleResult(string, int, luabind::object)> TupleHook;

typedef tuple<string, vector<TupleResult>, int> TupleVectorResult;
typedef hook::Hook<TupleVectorResult()> TupleVectorHook;


typedef script::BasicHook<SimpleHook> ScriptedSimpleHook;

struct ScriptedTupleKeys : public script::HookKeys<script::S, script::S, script::S>{
	ScriptedTupleKeys() : HookKeys("str", "num", "obj"){}
};
typedef script::BasicKeyedHook<TupleHook, ScriptedTupleKeys> ScriptedTupleHook;

struct ScriptedTupleVectorKeys : public script::HookKeys<
script::S, script::V<script::S, script::K<script::S, script::S, script::S>>, script::S>{
	ScriptedTupleVectorKeys() :
		HookKeys("str", v("tuples", "str", "num", "obj"), "num"){}
};
typedef script::BasicKeyedHook<TupleVectorHook, ScriptedTupleVectorKeys> ScriptedTupleVectorHook;

}

class ScriptedHookTest : public ::testing::Test{
protected:
	void SetUp()
	{
		L = luaL_newstate();
		luaL_openlibs(L);
		luabind::open(L);
	}

	void TearDown()
	{
		lua_close(L);
	}

	lua_State* L;
};

TEST_F(ScriptedHookTest, ReturnsTrivialValue){
	ASSERT_EQ(0, luaL_dostring(L, ""
		"function trivial(a, b)\n"
		"    return tostring(a) .. tostring(b)\n"
		"end"
	)) << lua_tostring(L, -1);

	ScriptedSimpleHook s;
	s.hookUp(luabind::globals(L)["trivial"]);

	try{
		ASSERT_EQ("91", SimpleHook(s)(9, 1));
	}catch(luabind::error& e){
		ADD_FAILURE() << lua_tostring(L, -1);
	}
}

TEST_F(ScriptedHookTest, TranslatesLuaTableIntoTuple){
	ASSERT_EQ(0, luaL_dostring(L, ""
		"object = {}\n"
		"function tuple(a, b, c)\n"
		"    return {str=a, num=b, obj=c}\n"
		"end"
	)) << lua_tostring(L, -1);

	ScriptedTupleHook s;
	s.hookUp(luabind::globals(L)["tuple"]);

	// note: luabind's object_proxy copy ctor is bogus, don't use auto
	luabind::object obj = luabind::globals(L)["object"];

	try{
		ASSERT_EQ(make_tuple("test", 13, obj), TupleHook(s)("test", 13, obj));
	}catch(luabind::error& e){
		ADD_FAILURE() << lua_tostring(L, -1);
	}
}

TEST_F(ScriptedHookTest, TranslatesLuaTableIntoVectorTuple){
	ASSERT_EQ(0, luaL_dostring(L, ""
		"object = {}\n"
		"object2 = {}\n"
		"function vectorTuple()\n"
		"  	 local v = {{str='inner', num=2, obj=object}, {str='inner2', num=3, obj=object2}}\n"
		"    return {str='<', tuples=v, num=-999}\n"
		"end"
	)) << lua_tostring(L, -1);;

	ScriptedTupleVectorHook s;
	s.hookUp(luabind::globals(L)["vectorTuple"]);

	luabind::object obj1 = luabind::globals(L)["object"];
	luabind::object obj2 = luabind::globals(L)["object2"];
	vector<TupleResult> v = { TupleResult("inner", 2, obj1), TupleResult("inner2", 3, obj2)};

	try{
		ASSERT_EQ(make_tuple(string("<"), v, -999), TupleVectorHook(s)());
	}catch(luabind::error& e){
		ADD_FAILURE() << lua_tostring(L, -1);
	}
}



