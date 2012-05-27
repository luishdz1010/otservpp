#ifndef OTSERVPP_SCRIPT_HOOK_OBJECT2TUPLE_HPP_
#define OTSERVPP_SCRIPT_HOOK_OBJECT2TUPLE_HPP_

#include <tuple>
#include <vector>
#include <boost/lexical_cast.hpp>
#include "hookdcl.hpp"
#include "../scriptexcep.hpp"

OTSERVPP_NAMESPACE_SCRIPT_HOOK_BEGIN

typedef ErrorInfo<struct TagErrorInfoTranslationKey, std::string> ErrorInfoTranslationKey;

/*! Utility class for Lua to C++ table conversion
 * This may be generalized but at the moment is not required.
 * TODO move this out from hooks if required elsewhere
*/
class ObjectToTuple{
public:
	/*! Translates a obj lua table to a corresponding Result tuple, using the keys tuple
	 * for indexing the table. Keys and Result should follow a mirror structure:
	 * \code
	 * 		Result = tuple<string, int, myLuaExportedClass> && Keys = tuple<key1, key2, key3>
	 * \endcode
	 * where keyN can be any Lua indexable value (mostly strings).
	 * Vector of convertible elements are supported:
	 * \code
	 *  	Result = tuple<string, int, vector<myLuaExportedClass>> &&
	 *  	Keys   = tuple<key1, key2, key3>
	 * \endcode
	 * Vector of tuples are supported with a pair tuple element:
	 * \code
	 *  	Result = tuple<string, int, vector<tuple<string, myLuaExportedClass>>> &&
	 *  	Keys   = tuple<key2, key2, pair<key3, tuple<innerKey1, innerKey2>>>
	 * \encode
	 */
	template <class Result, class Keys>
	static Result translate(const luabind::object& obj, const Keys& keys)
	{
		expectTable(obj);
		Result r;
		KeyRecorder<void, typename std::tuple_element<std::tuple_size<Keys>::value-1, Keys>::type>
			keyRecorder {std::get<std::tuple_size<Keys>::value-1>(keys)};

		fill<std::tuple_size<Result>::value-1>(r, obj, keys, keyRecorder);
		return r;
	}

	template <class Result, class Keys>
	static std::string getHumanReadableFormat(const Keys& keys)
	{
		return ">>error<<";
	}

private:
	// Highly templated code follows, it basically traverses lua objects recursively while
	// attempting to convert them to the required types, implementation can be optimized tho


	template <class P, class K>
	struct KeyRecorderBase{
		typedef P parent;
		typedef K key;
	};

	template <class Key>
	static std::string extractKey(Key& k)
	{
		return k;
	}

	template <class Key, class ChildKeys>
	static std::string extractKey(const std::pair<Key, ChildKeys>& ks)
	{
		return ks.first;
	}

	template <class P, class K>
	struct KeyRecorder : public KeyRecorderBase<P, K>{
		const P& p;
		const K& key;

		KeyRecorder(const P& p_, const K& k_) : p(p_), key(k_){}

		template <class OldKey, class NewKey>
		KeyRecorder(const KeyRecorder<P, OldKey>& o, const NewKey& n) : p(o.p), key(n){}

		// table.value
		operator std::string() const {
			return static_cast<std::string>(p) + "." + extractKey(key);
		}
	};

	template <class K>
	struct KeyRecorder<void, K> : public KeyRecorderBase<void, K>{
		const K& key;

		KeyRecorder(const K& k_) : key(k_) {}

		template <class OldKey, class NewKey>
		KeyRecorder(const KeyRecorder<void, OldKey>& o, const NewKey& n) : key(n) {}

		// value && table[n]
		operator std::string() const { return extractKey(key); }
	};

	template <class P, class K>
	struct IndexKeyRecorder : public KeyRecorderBase<P, K>{
		const P& p;
		const K& key;
		int i;

		IndexKeyRecorder(const P& p_, const K& k_, int i_) : p(p_), key(k_), i(i_) {}

		template <class OldKey, class NewKey>
		IndexKeyRecorder(const IndexKeyRecorder<P, OldKey>& o, const NewKey& n) : p(o.p), key(n), i(o.i) {}

		// table[n].value
		operator std::string() const{
			return static_cast<std::string>(p) + indexToString(i) + "." + extractKey(key);
		};
	};

	template <int pos, class Result, class Keys, class P, class K, template<class, class> class KeyRec>
	static typename std::enable_if<(pos > 0)>::type
	fill(Result& r, const luabind::object& table, const Keys& keys, const KeyRec<P, K>& krec)
	{
		put(krec, table, std::get<pos>(r));
		fill<pos-1>(r, table, keys, KeyRec<P, typename std::tuple_element<pos-1, Keys>::type>
										{krec, std::get<pos-1>(keys)});
	}

	template <int pos, class Result, class Keys, class KeyRec>
	static typename std::enable_if<(pos == 0)>::type
	fill(Result& r, const luabind::object& table, const Keys& keys, const KeyRec& krec)
	{
		put(krec, table, std::get<0>(r));
	}

	// Single element
	template <class KeyRec, class T>
	static void	put(const KeyRec& krec, const luabind::object& table, T& item)
	{
		try{
			item = objectCast<T>(table[krec.key]);
		}catch(Exception& e){
			e << ErrorInfoTranslationKey(krec);
			throw;
		}
	}

	// Vector of convertible elements
	template <class KeyRec, class T>
	static void put(const KeyRec& krec, const luabind::object& table, std::vector<T>& v)
	{
		auto& inner = table[krec.key];

		expectTableAndAppendKey(inner, krec);

		int i = 1;
		try{
			for(luabind::iterator it(inner), end; it != end; ++it){
				v.push_back(objectCast<T>(*it));
				++i;
			}
		}catch(Exception& e){
			e << ErrorInfoTranslationKey(krec + indexToString(i));
		}
	}

	// key = pair<string, tuple<string, string, ...>>
	// Vector of tuple elements
	template <class KeyRec, class... T>
	static void	put(const KeyRec& krec, const luabind::object& table, std::vector<std::tuple<T...>>& v)
	{
		typedef IndexKeyRecorder<KeyRec, decltype(std::get<sizeof...(T)-1>(krec.key.second))> ChildKeyRec;

		auto inner = table[krec.key.first];
		auto& innerKeys = krec.key.second;

		expectTableAndAppendKey(inner, krec);

		int i = 1;
		for(luabind::iterator it(inner), end; it != end; ++it){
			ChildKeyRec childkRec {krec, std::get<sizeof...(T)-1>(innerKeys), i};
			expectTableAndAppendKey(*it, childkRec);
			fill<sizeof...(T)-1>(*v.emplace(v.end()), *it, innerKeys, childkRec);
			++i;
		}
	}

	template <class Key>
	static void expectTableAndAppendKey(const luabind::object& table, const Key& key)
	{
		try{
			expectTable(table);
		}catch(Exception& e){
			e << ErrorInfoTranslationKey(key);
			throw;
		}
	}

	static std::string indexToString(int i)
	{
		return std::string("[") + boost::lexical_cast<std::string>(i) + "]";
	}
};

OTSERVPP_NAMESPACE_SCRIPT_HOOK_END

#endif // OTSERVPP_SCRIPT_HOOK_OBJECT2TUPLE_HPP_
