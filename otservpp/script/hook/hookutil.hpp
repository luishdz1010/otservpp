#ifndef OTSERVPP_SCRIPT_HOOK_HOOKUTIL_HPP_
#define OTSERVPP_SCRIPT_HOOK_HOOKUTIL_HPP_

#include <tuple>
#include <utility>
#include "hook.hpp"

OTSERVPP_NAMESPACE_SCRIPT_HOOK_BEGIN

// Helpers for translation keys declarations
template <class T, class U>
using V = std::pair<T, U>;

template <class... T>
using K = std::tuple<T...>;

typedef const char* S;

/* Defines the specific set of keys that Lua will use as table indexes when returning a
 * complex table from a hook call.
 * \see Object2Tuple BasicHook
 */
template <class... T>
class HookKeys{
public:
	typedef std::tuple<T...> KeyTuple;

	template <class... Keys>
	HookKeys(Keys&&... keys_) :
		keys(std::forward<Keys>(keys_)...)
	{}

	const KeyTuple& operator()(const luabind::object&)
	{
		return keys;
	}

protected:
	// use for in-line vector key declaration
	template <class VKey, class... EKey>
	static std::pair<typename std::decay<VKey>::type, std::tuple<typename std::decay<EKey>::type...>>
	v(VKey&& vkey, EKey&&... ekey)
	{
		return std::make_pair(
				std::forward<VKey>(vkey),
				std::make_tuple(std::forward<EKey>(ekey)...));
	}

private:
	KeyTuple keys;
};

OTSERVPP_NAMESPACE_SCRIPT_HOOK_END

#endif // OTSERVPP_SCRIPT_HOOK_HOOKUTIL_HPP_
