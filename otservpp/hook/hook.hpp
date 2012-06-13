#ifndef OSERVPP_HOOK_HOOK_HPP_
#define OSERVPP_HOOK_HOOK_HPP_

#include <functional>
#include "hookdcl.hpp"

OTSERVPP_NAMESPACE_HOOK_BEGIN

/*! The Hook template class represents a customization point in an arbitrary position in the
 * application that can be easily implemented by others without the need to modify the
 * application itself. A hook is modeled after a function taking input parameters and
 * returning useful values to the application (via simple return values or tuples), most hooks
 * are likely to be implemented through scripts, but these can be directly in C++ if
 * performance is a concern.
 */
template <class R, class... P>
class Hook<R(P...)>{
public:
	typedef R Result;
	typedef R result_type;

	template <class U>
	Hook(U&& h) :
		hook(std::forward<U>(h))
	{}

	Hook(const Hook&) = default;
	Hook(Hook&&) = default;

	template <class U>
	Hook& operator=(U&& h)
	{
		hook = std::forward<U>(h);
		return *this;
	}

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
