#ifndef OTSERVPP_SCRIPT_HOOK_HOOK_HPP_
#define OTSERVPP_SCRIPT_HOOK_HOOK_HPP_

#include <glog/logging.h>
#include <luabind/function.hpp>
#include <luabind/class_info.hpp>
#include "hookdcl.hpp"
#include "object2tuple.hpp"

OTSERVPP_NAMESPACE_SCRIPT_HOOK_BEGIN

typedef ErrorInfo<struct TagErrorInfoTranslation, std::string> ErrorInfoValueTranslation;

/*! Represents a scripted hook implementation.
 * \see otservpp::hook::Hook
 */
class Hook{
public:
	virtual ~Hook(){};

	/*! Instructs the hook to hook up on the given object.
	 * The passed object will be valid until the hook's unhook method is called. Furthermore,
	 * its guaranteed that the object will always be callable.
	 */
	virtual void hookUp(const luabind::object&) = 0;

	/*! If you store any copy of the object passed to hookUp (you probably will), set it to null
	 * in this method.
	 */
	virtual void unhook() = 0;
};

/*! Base for the basic implementation of script Hooks.
 * The script hook implementation stores a copy of a given Lua callable object, and calls it
 * whenever the hook is called.
 *
 * Argument passing is done automatically for every luabind compatible type, even when the main
 * hook changes its interface.
 *
 * Argument retrieval is also handled automatically when the expected result of the hook is a
 * C++ single value. When returning complex values, a Lua table should be used. Most hooks
 * interfaces expect either a single standard value or a std::tuple as return values. For the of
 * later case, the returned Lua table is translated into is corresponding tuple with the help
 * a keys tuple, that tells the translator where to look in the resulting table. If the hook
 * interface doens't expect a tuple nor a standard value as its result, the expected result type sh
 * must be registered to luabind.
 *
 * It automatically manages the lifetime of a script function object. Use BasicHook or
 * BasicKeyedHook template subclasses for defining new script hooks.
 */
template <class Derived, class H>
class BaseBasicHook : public Hook{
public:
	typedef typename H::Result Result;

	void hookUp(const luabind::object& f) final override
	{
		func = f;
		attach();
	}

	void unhook() final override
	{
		detach();
		func = luabind::object();
	}

	luabind::object& getScriptFunction()
	{
		return func;
	}

	template <class... Args>
	Result operator()(Args&&... args)
	{
		try{
			return static_cast<Derived*>(this)->
				translate(callScriptFunction(std::forward<Args>(args)...));
		}catch(Exception& e){
			std::ostringstream s;
			//s << "in execution of script hook \"" << FunctionInfo(func).getLocation() << "\"\n"
			s << "in execution of script hook \"" << "<todo>" << "\"\n";

			if(std::string* what = getErrorInfo<ErrorInfoValueTranslation>(e))
				s << *what;
			if(std::string* argumentInfo = getErrorInfo<ErrorInfoInvalidArgument>(e))
				s << *argumentInfo;

			LOG(ERROR) << s.str();
			throw;
		}
	}

protected:
	//~BaseBasicHook() = default;

	// Subclasses can't override hookUp anymore, declare a void attach() method instead
	virtual void attach(){}

	// Subclasses can't override unhook anymore, define a void detach() method instead
	virtual void detach(){}

	template <class Ret, class... Args>
	Ret callScriptFunction(Args&&... args)
	{
		return luabind::call_function<Ret>(func, std::forward<Args>(args)...);
	}

	template <class... Args>
	luabind::object callScriptFunction(Args&&... args)
	{
		return callScriptFunction<luabind::object>(std::forward<Args>(args)...);
	}

private:
	luabind::object func;
};

/*! Used for hooks with trivial return types.
 * This works for every type that can be translated by luabind (including custom registered
 * types)
 */
template <class H, typename std::enable_if<
		luabind::default_converter<typename H::Result>::is_native::value, int>::type = 0>
class BasicHook : public BaseBasicHook<BasicHook<H>, H>{
public:
	typedef typename H::Result Result;

	Result translate(const luabind::object& obj)
	{
		return objectCast<Result>(obj);
	}
};

/*! Used for hooks with complex return types.
 * The returned Lua table is translated into the expected result (a tuple most of the time).
 * \see Object2Tuple
 */
template <class H, class Keys, typename std::enable_if<
		(std::tuple_size<typename H::Result>::value > 0), int>::type = 0>
class BasicKeyedHook : public BaseBasicHook<BasicKeyedHook<H, Keys>, H>{
public:
	typedef typename H::Result Result;

	Result translate(const luabind::object& obj)
	{
		const auto& ks = keys(obj);

		try{
			return ObjectToTuple::translate<Result>(obj, ks);
		}catch(Exception& e){
			std::ostringstream s;
			s << "expected a return value with the following the structure \n\""
				<< ObjectToTuple::getHumanReadableFormat<Result>(ks);

			if(std::string* key = getErrorInfo<ErrorInfoTranslationKey>(e))
				s << "\"\nerror at key \"" << *key << "\" ";

			e << ErrorInfoValueTranslation(s.str());
			throw;
		}
	}

private:
	Keys keys;
};

OTSERVPP_NAMESPACE_SCRIPT_HOOK_END

#endif // OTSERVPP_SCRIPT_HOOK_HOOK_HPP_
