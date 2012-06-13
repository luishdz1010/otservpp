#ifndef OSERVPP_HOOK_HOOKDCL_HPP_
#define OSERVPP_HOOK_HOOKDCL_HPP_

#include <stdexcept>

#define OTSERVPP_NAMESPACE_HOOK_BEGIN namespace otservpp{ namespace hook{
#define OTSERVPP_NAMESPACE_HOOK_END } }

/// Handy macro for declaring Hooks alongside an human-readable name
#define OTSERVPP_DEF_HOOK(HookName, FuncType) \
struct HookName : public otservpp::hook::Hook<FuncType>{ \
	template <class T> \
	HookName(T&& h) : otservpp::hook::Hook<FuncType>(std::forward<T>(h)){} \
	\
	static const char* getName(){ return #HookName " hook"; } \
}

OTSERVPP_NAMESPACE_HOOK_BEGIN

template <class T> class Hook;

OTSERVPP_NAMESPACE_HOOK_END

#endif // OSERVPP_HOOK_HOOKDCL_HPP_
