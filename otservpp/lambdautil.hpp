#ifndef LAMBDAUTIL_HPP_
#define LAMBDAUTIL_HPP_

#include <utility>

/*! \file
 * Contains some utilities and compiler workarounds for efficient use of C++11 lamdbas
 */
template <class... T> struct def_arg;

template <class T1> struct def_arg<T1>{ typedef T1 arg1_type; };

template <class T1, class T2> struct def_arg<T1, T2> :
	public def_arg<T1>{ typedef T2 arg2_type; };

template <class T1, class T2, class T3> struct def_arg<T1, T2, T3> :
	public def_arg<T1, T2>{ typedef T3 arg3_type; };

/*template <int N, class... T> struct def_arg;

template <class T1> struct def_arg<1, T1>{ typedef T1 arg1_type; };

template <class T1, class T2> struct def_arg<2, T1, T2> :
	public def_arg<1, T1>{ typedef T2 arg2_type; };

template <class... T, class T3> struct def_arg<3, T..., T3> :
	public def_arg<2, T...>{ typedef T3 arg3_type; };

template <class... T, class T4> struct def_arg<4, T..., T4> :
	public def_arg<3, T...>{ typedef T4 arg4_type; };*/


template <class Lambda> struct function_traits :
public function_traits<decltype(&Lambda::operator())> {};

/// similar to boost::function_traits but works with everything
template <class Ret, class... Args>
struct function_traits<Ret(Args...)> : public def_arg</*sizeof...(Args), */Args...>{
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


// helpers for Lambda
template <class T>
struct LambdaTraits : public function_traits<T> {};

/*! Variadic captures, move-enabled lambdas
 * C++ lambdas cannot capture by "move", this is sometimes an inconvenient. Here we simply
 * emulate captures by storing the arguments inside the object and passing them to the
 * lambda when it is executed (alongside with the arguments received). This works similar to
 * std::bind. It also allows us to workaround some GCC (and possible other untested compilers)
 * limitations while unpacking parameter packs on lambda captures.
 * Some examples:
 * \code
 *  template <class... Args>
 * 	void foo(Args&&... args)
 * 	{
 * 		// forwards args... to bar(), a capture(move/copy) of args must be done but after that
 * 		// we can freely move-out the pack
 * 		lambdaBind([](Args&&... args){
 * 			bar(std::move<Args>(args)...);
 * 		}, std::forward<Args>(args)...)();
 * 	}
 * 	...
 * 	// parameters passed in the call site are appended to the right
 * 	lambdaBind([](int one, int two, int three){}, 1, 2)(3);
 *	...
 *	lambdaBind(std::move(foo), [](Foo&& foo){ // take by && or Foo if moving
 *		return bar(std::move(foo)); // works with return types
 *	});
 * \endcode
 */
template <class L, class... Captures>
class Lambda{
	static_assert(std::is_same<typename std::remove_reference<L>::type, L>::value,
				"lambda type must not be a reference");

	// unpacking of tuples
	template <int...> struct Seq{};
	template <int n, int... s> struct Counter : Counter<n-1, n-1, s...>{};
	template <int... s> struct Counter<0, s...>{ typedef Seq<s...> type; };

public:
	typedef typename LambdaTraits<L>::result_type result_type;
	typedef typename Counter<sizeof...(Captures)>::type CaptureSeq;
    typedef std::tuple<typename std::remove_reference<Captures>::type...> StoredTypes;
    typedef std::tuple<Captures...> OriginalTypes;

	template<class U, class Arg1, class... Args>
	Lambda(U&& lambda_, Arg1&& arg1, Args&&... args) :
		captures(std::forward<Arg1>(arg1), std::forward<Args>(args)...),
		lambda(std::forward<U>(lambda_))
	{}

	// has to be defined for some reason in gcc
	Lambda(Lambda&& o) :
		captures(std::move(o.captures)),
		lambda(std::move(o.lambda))
	{}

	Lambda(const Lambda&) = default;

	template <class... Args>
	result_type operator()(Args&&... args)
	{
		return call(CaptureSeq(), std::forward<Args>(args)...);
	}

private:
	template <int... seq, class... Args>
	result_type call(Seq<seq...>, Args&&... args)
	{
		return lambda(
			std::forward<typename std::tuple_element<seq, OriginalTypes>::type>
				(std::get<seq>(captures))...,
			std::forward<Args>(args)...
		);
	}

	StoredTypes captures;
	L lambda;
};

// useful for nested Lambda structures
template <class T, class... Captures>
struct LambdaTraits<Lambda<T, Captures...>> : public LambdaTraits<T>{};

template <class L, class... Captures>
inline Lambda<typename std::remove_reference<L>::type, Captures...>
lambdaBind(L&& lambda, Captures&&... args)
{
	return {std::forward<L>(lambda), std::forward<Captures>(args)...};
}

/*! Simple class for doing some RAII with lambdas
 * Using this like \code static auto foo = makeScopedScoperation(A, B); \endcode you can
 * statically-thread-safe call A once while entering the scope and B during the program
 * shutdown. But it is probably more useful for tiny in-need-for exception safe code :)
 */
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
