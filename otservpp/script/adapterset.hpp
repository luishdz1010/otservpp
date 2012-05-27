#ifndef OTSERVPP_SCRIPT_ADAPTERSET_H_
#define OTSERVPP_SCRIPT_ADAPTERSET_H_

#include <unordered_set>
#include <glog/logging.h>
#include <boost/assert.hpp>
#include "scriptptr.hpp"
#include "adapter/adapter.hpp"

/// Inserts the given adapter to the StaticAdapters set. Don't use this in header files
#define OTSERVPP_REG_STATIC_ADAPTER(Adaper) \
	static const bool Adaper##dummy = StaticAdapters::insert<Adaper>();


/// Declaring and registering of simple TypeAdapters in one statement
#define OTSERVPP_STATIC_ADAPTER_T(Adapter, Adaptee) \
	OTSERVPP_NAMESPACE_SCRIPT_BEGIN \
	struct Adapter : public TypeAdapter<Adaptee>{ \
		void apply(lua_State* L) override; \
	}; \
	OTSERVPP_NAMESPACE_SCRIPT_END \
	OTSERVPP_REG_STATIC_ADAPTER(Adapter) \
	void OTSERVPP_NAMESPACE_SCRIPT::Adapter::apply(lua_State* L)


OTSERVPP_NAMESPACE_SCRIPT_BEGIN

// Don't replace these with std::hash and std::equal_to specializations, we only want
// custom behavior for AdapterSet

struct AdapterPtrHash{
	size_t operator()(const AdapterPtr& adapter) const{
		return std::hash<std::string>()(adapter->getAdaptee());
	}
};

struct AdapterPtrEquality{
	bool operator()(const AdapterPtr& lhs, const AdapterPtr& rhs) const{
		return *lhs == *rhs;
	}
};

/* Manages a collection of adapters that register C++ classes and functions to Lua.
 * Use this class to hold various adapters before applying them to a lua_State.
 *
 * To apply all the registered static adapters you do something like:
 * 	\code
 * 		apply(StaticAdapters::getAll(), L);
 * 	\endcode
 * Inserting more adapters:
 * 	\code
 * 		AdapterSet adapters(StaticAdapters::getAll());
 * 		adapters.insert(...); // custom insertion
 * 		apply(adapters, L);
 * 	\endcode
 * \sa Adapter
 */
typedef std::unordered_set<AdapterPtr, AdapterPtrHash, AdapterPtrEquality> AdapterSet;


/* Contains the set of stateless adapters that don't need extra parameters to be constructed,
 * and therefore can be one-time constructed.
 * The majority of adapters that adapts only C++ types fall into this category. You should only
 * register here truly state-less adapters, because only one instance of them will be ever
 * created.
 * \sa AdapterSet
 */
class StaticAdapters{
public:

	StaticAdapters() = delete;

	/* Registers an adapter to the static adapter set.
	 * Use the provided macros OTSERVPP_REG_STATIC_ADAPTER or OTSERVPP_STATIC_ADAPTER instead.
	 * \return dummy boolean for static insertion
	 */
	template <class T>
	static bool insert(){
		bool insertionTookPlace = getSet().insert(std::make_shared<T>()).second;
		BOOST_ASSERT_MSG(insertionTookPlace,
				"Registering a static script adapter with duplicated adaptee");
		return true;
	}

	/// Returns the set of all the adapters registered with insert
	static const AdapterSet& getAll()
	{
		return getSet();
	}

private:
	static AdapterSet& getSet()
	{
		static AdapterSet adapters;
		return adapters;
	}
};


/* \relates AdapterSet
 * Applies all the adapters in the given set the given Lua state.
 */
inline void apply(const AdapterSet& adapters, lua_State* L)
{
	for(auto& adapter : adapters){
		VLOG(1) << "Applying " << adapter->getAdaptee() << " script adapter to Lua state " << L;
		adapter->apply(L);
	}
}

OTSERVPP_NAMESPACE_SCRIPT_END

#endif // OTSERVPP_SCRIPT_ADAPTERSET_H_
