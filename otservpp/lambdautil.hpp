#ifndef LAMBDAUTIL_HPP_
#define LAMBDAUTIL_HPP_

#include <utility>

/*! \file
 * Contains some utilities and compiler workarounds for efficient use of C++11 lamdbas
 */

template <class Lambda> struct function_traits :
public function_traits<decltype(&Lambda::operator())> {};

/// similar to boost::function_traits but works with everything
template <class Ret, class... Args>
struct function_traits<Ret(Args...)>{
	typedef Ret result_type;
	enum{ arity = sizeof...(Args) };
};

template <class Ret, class... Args>
struct function_traits<Ret(*)(Args...)> : public function_traits<Ret(Args...)> {};

template <class Ret, class T, class... Args>
struct function_traits<Ret(T::*)(Args...)> : public function_traits<Ret(Args...)> {};

template <class Ret, class T, class... Args>
struct function_traits<Ret(T::*)(Args...) const> : public function_traits<Ret(Args...)> {};

template <class T>
struct function_traits<std::function<T>> : public function_traits<T> {};

template <class T>
struct function_traits<T&> : public function_traits<typename std::remove_reference<T>::type> {};


/*! Variadic captures, move-enabled lambdas
 * C++ lambdas cannot capture by "move", this is sometimes an inconvinient. Here we simply
 * emulate captures by storing the arguments inside the object and passing them to the
 * lambda when it is executed (alongside with the arguments received). This works like
 * std::bind but its less verbose. Some examples:
 * \code
 *  template <class... Args>
 * 	void foo(Args&&... args)
 * 	{
 * 		// perfect forwards args... to bar()
 * 		makeLambda(std::forward<Args>(args)..., [](Args&... args){ // TAKE BY REF NOT BY &&
 * 			bar(std::forward<Args>(args)...);
 * 		})();
 * 	}
 * 	...
 * 	// parameters passed in the call site are appended to the right
 * 	makeLambda(1, 2, [](int one, int two, int three){})(3);
 *	...
 *	makeLambda(std::move(foo), [](Foo& foo){
 *		return bar(std::move(foo)); // works with return types
 *	});
 * \endcode
 */
template <class L, class... Captures>
class Lambda{
	// unpacking of tuples
	template <int...> struct Seq{};
	template <int n, int... s> struct Counter : Counter<n-1, n-1, s...>{};
	template <int... s> struct Counter<0, s...>{ typedef Seq<s...> type; };

public:
	typedef typename function_traits<L>::result_type result_type;
    typedef std::tuple<typename std::remove_reference<Captures>::type...> StoredTypes;
    typedef std::tuple<Captures...> OriginalTypes;

	template<class U, class... Args>
	Lambda(U&& lambda_, Args&&... args) :
		captures(std::forward<Args>(args)...),
		lambda(std::forward<U>(lambda_))
	{}

	template <class... Args>
	result_type operator()(Args&&... args)
	{
		return call(typename Counter<sizeof...(Captures)>::type(), std::forward<Args>(args)...);
	}

private:
	template <int... seq, class... Args>
	result_type call(Seq<seq...>, Args&&... args)
	{
		//return lambda(std::get<seq>(captures)..., std::forward<Args>(args)...);
		return lambda(
				std::forward<typename std::tuple_element<seq, OriginalTypes>::type>
					(std::get<seq>(captures))...,
				std::forward<Args>(args)...
		);
	}

	StoredTypes captures;
	L lambda;
};

template <class L, class... Captures>
inline Lambda<typename std::remove_reference<L>::type, Captures...>
makeLambda(L&& lambda, Captures&&... args)
{
	return {std::forward<L>(lambda), std::forward<Captures>(args)...};
}

template <class Destructor>
class ScopedOperation{
public:
	template <class U, class V>
	ScopedOperation(U&& c, V&& d) :
		exit(std::forward<V>(d))
	{
		c();
	}

	/*template <class U>
	ScopedOperation(U&& d) :
		exit(std::forward<U>(d))
	{}*/

	~ScopedOperation()
	{
		exit();
	}

private:
	Destructor exit;
};

template <class OnEnter, class OnExit>
ScopedOperation<OnExit> makeScopedOperation(OnEnter&& c, OnExit&& d)
{
	return {std::forward<OnEnter>(c), std::forward<OnExit>(d)};
}

#endif // LAMBDAUTIL_HPP_ 
