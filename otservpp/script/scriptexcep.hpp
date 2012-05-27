#ifndef OTSERV_SCRIPT_SCRIPTEXCEP_HPP_
#define OTSERV_SCRIPT_SCRIPTEXCEP_HPP_

#include <sstream>
#include <luabind/object.hpp>
#include <luabind/class_info.hpp>
#include "scriptdcl.hpp"
#include "../exception.hpp"

OTSERVPP_NAMESPACE_SCRIPT_BEGIN

typedef ErrorInfo<struct TagErrorInfoInvalidArgument, std::string> ErrorInfoInvalidArgument;

/// Indicates an error while processing a Lua script
struct ScriptException : virtual Exception{};

struct TableTranslationException : virtual ScriptException{};

/// Indicates that a C++ function expected a Lua type and was given a different one
struct InvalidScriptArgument : virtual ScriptException{};

/// Returns the Lua type name for the registered class or native type T
template <class T>
inline typename std::enable_if<std::is_class<T>::value, const char*>::type
getTypeName(lua_State* L)
{
	// only way to do this :-(
	auto* cls = luabind::detail::class_registry::get_registry(L)->find_class(typeid(T));
	if(!cls)
		throw std::logic_error("trying to get the Lua type name of a unregistered class");
	return cls->name();
}

/*template <class T>
inline const char* getTypeName<std::shared_ptr<T>>(lua_State* L)
{
	return getTypeName<T>();
}*/

template <class T>
inline typename std::enable_if<std::is_arithmetic<T>::value, const char*>::type
getTypeName(lua_State* L)
{
	return lua_typename(L, LUA_TNUMBER);
}

template <>
inline const char* getTypeName<bool>(lua_State* L)
{
	return lua_typename(L, LUA_TBOOLEAN);
}

template <>
inline const char* getTypeName<std::string>(lua_State* L)
{
	return lua_typename(L, LUA_TSTRING);
}

inline const char* getTypeName(const luabind::object& obj)
{
	return "SOME";//luabind::get_class_info().name;
}

template <class T>
T objectCast(const luabind::object& obj)
{
	try{
		return luabind::object_cast<T>(obj);
	}catch(luabind::cast_failed& e){
		/*throwException(InvalidScriptArgument()
				<< ErrorInfoInvalidArgument(lua_tostring(obj.interpreter(), -1)));*/

		/*std::ostringstream s;
		s << "value of type [" << getTypeName<T>(obj.interpreter())
				<< "] expected; given value is of type [" << getTypeName(obj) << "]";*/

		throwException(InvalidScriptArgument()
			<< ErrorInfoInvalidArgument(
					std::string("[from objectCast] ") + e.what() + "  " +
					std::string(lua_tostring(obj.interpreter(), -1))));
	}
}


inline bool isAnyOfType(int given, int expected)
{
	return given == expected;
}

template <class... Other>
inline bool isAnyOfType(int given, int expected, Other... types)
{
	return isAnyOfType(given, expected) || isAnyOf(given, types...);
}

/// Returns true if any of the types provided match the Lua obj type
template <class... Other>
inline bool isAnyOfType(const luabind::object& obj, Other... types)
{
	return isAnyOfType(luabind::type(obj), types...);
}

inline std::string concatTypes(lua_State* L, int type)
{
	return lua_typename(L, type);
}

template <class... Other>
inline std::string concatTypes(lua_State* L, int type1, Other... types)
{
	return concatTypes(L, type1) + ", " + concatTypes(L, types...);
}

/// Returns a string containing a list of Lua types separated by comma
template <class... Other>
inline std::string concatTypes(const luabind::object& obj, Other... types)
{
	return concatTypes(obj.interpreter(), luabind::type(obj), luabind::type(types)...);
}

inline bool isTable(const luabind::object& obj)
{
	return isAnyOfType(obj, LUA_TTABLE);
}

/// Throws a InvalidScriptArgument exception if !isAnyOf(obk, types...)
template <class... Expected>
inline void expectAnyOf(const luabind::object& obj, Expected... types)
{
	if(!isAnyOfType(obj, types...)){
		std::ostringstream s;
		s << "value of type [" << concatTypes(obj.interpreter(), types...)
			<< "] expected; given value is of type [" << concatTypes(obj) << "]";

		throwException(InvalidScriptArgument()
			<< ErrorInfoInvalidArgument(s.str()));
	}
}

/// Throws a InvalidScriptArgument exception if type(given) != LUA_TFUNCTION
inline void expectFunction(const luabind::object& given)
{
	expectAnyOf(given, LUA_TFUNCTION);
}

/// Throws a InvalidScriptArgument exception if type(given) != LUA_TTABLE
inline void expectTable(const luabind::object& given)
{
	expectAnyOf(given, LUA_TTABLE);
}

OTSERVPP_NAMESPACE_SCRIPT_END

#endif // OTSERV_SCRIPT_SCRIPTEXCEP_HPP_
