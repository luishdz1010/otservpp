#ifndef OSERVPP_HOOK_HOOK_HPP_
#define OSERVPP_HOOK_HOOK_HPP_

#include <functional>
#include <typeinfo>
#include <glog/logging.h>
#include "hookdcl.hpp"

OTSERVPP_NAMESPACE_HOOK_BEGIN

template <class R, class... P>
class Hook<R(P...)>{
public:
	typedef R Result;

	template <class U>
	Hook(U&& h) :
		hook(std::forward<U>(h))
	{}

	template <class... Args>
	Result operator()(Args&&... args)
	{
		return hook(std::forward<Args>(args)...);
	}

private:
	std::function<Result(P...)> hook;
};

OTSERVPP_NAMESPACE_HOOK_END

#endif // OSERVPP_HOOK_HOOK_HPP_
