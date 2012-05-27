#ifndef OTSERVPP_SCRIPT_ADAPTER_HPP_
#define OTSERVPP_SCRIPT_ADAPTER_HPP_

#include <typeinfo>
#include <string>
#include "../scriptdcl.hpp"

OTSERVPP_NAMESPACE_SCRIPT_BEGIN

/* Represents a unit of functionality exposed from C++ to Lua.
 * For convenience reasons, all the exposed functions, classes and objects exposed to Lua are
 * declared as adapters, every adapter exposes or "adapts" several different constructions from
 * C++ to Lua.
 *
 * This makes extremely easy to expose new functionality to Lua in a modular fashion. And added
 * bonus is the ability to unit-test each Adapter.
 *
 * There're two kinds of adapters, "static adapter" and "common adapter", the former type
 * do not require extra info to do their job(i.e. arguments), and more importantly, they're
 * stateless, the later type does need more information and is almost never stateless. All static
 * adapters are registered and constructed before the server starts using the static class
 * StaticAdapters, later you can access them through StaticAdapters::getAll(). Common adapters
 * or simply adapters can't be default constructed and therefore need to be created separately
 * in the code.
 *
 * Most type-related adapters are static. Some examples of common adapters:
 * 	\li Global objects in lua like map, game, etc
 * 	\li Global functions that work by delegating the work to given object, like scheduler
 *
 * 	\sa StaticAdapters AdapterSet ::apply()
 */
class Adapter{
public:
	virtual ~Adapter() = default;

	/// Applies the adapter functionality to the given Lua state
	virtual void apply(lua_State* L) = 0;

	/// Returns the name of the adaptee, this should be unique for every adapter
	/// its used for hashing and comparison in AdapterSet
	virtual const std::string& getAdaptee() const = 0;

	bool operator==(const Adapter& rhs)
	{
		return getAdaptee() == rhs.getAdaptee();
	}
};

/// Base class implementing adaptee retrieval
class BasicAdapter : public Adapter{
public:
	template <class String>
	BasicAdapter(String&& str) :
		adaptee(std::forward<String>(str))
	{}

	const std::string& getAdaptee() const final override
	{
		return adaptee;
	}

private:
	std::string adaptee;
};

/// Subclass TypeAdapter if you want to exporting a type/function to Lua, most type adapters are
/// static, so you should use OTSERVPP_REG_STATIC_ADAPTER for your derived type.
template <class T>
class TypeAdapter : public BasicAdapter{
public:
	typedef T Adaptee;

	TypeAdapter() :
		BasicAdapter(typeid(Adaptee).name())
	{}
};

// Subclass GlobalInstanceAdapter when exporting a variable into lua's global table.
template <class T>
class GlobalInstanceAdapter : public BasicAdapter{
public:
	typedef T Adaptee;

	GlobalInstanceAdapter(std::string&& instanceName) :
		BasicAdapter(std::string(typeid(Adaptee).name()) + "_G-" + std::move(instanceName))
	{}
};

OTSERVPP_NAMESPACE_SCRIPT_END

#endif // OTSERVPP_SCRIPT_ADAPTER_HPP_
