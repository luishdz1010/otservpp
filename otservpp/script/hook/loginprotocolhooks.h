#ifndef OTSERVPP_SCRIPT_HOOK_LOGINPROTOCOLHOOKS_H_
#define OTSERVPP_SCRIPT_HOOK_LOGINPROTOCOLHOOKS_H_

#include "hookutil.hpp"
#include "../../hook/loginprotocolhooks.hpp"

OTSERVPP_NAMESPACE_SCRIPT_HOOK_BEGIN

struct AccountLoginSuccedKeys : public HookKeys<S, V<S,K<S,S,S,S>>, S> {
	AccountLoginSuccedKeys() :
		HookKeys("motd", v("characters", "name", "world", "ip", "port"), "premiumEnd"){}
};

typedef BasicKeyedHook<hook::AccountLoginSucced, AccountLoginSuccedKeys> AccountLoginSucced;
typedef BasicHook<hook::AccountLoginFailed> AccountLoginFailed;

OTSERVPP_NAMESPACE_SCRIPT_HOOK_END

#endif // OTSERVPP_SCRIPT_HOOK_LOGINPROTOCOLHOOKS_H_
