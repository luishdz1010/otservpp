#ifndef OTSERVPP_HOOK_LOGINPROTOCOLHOOKS_HPP_
#define OTSERVPP_HOOK_LOGINPROTOCOLHOOKS_HPP_

#include <string>
#include <vector>
#include "../forwarddcl.hpp"
#include "hook.hpp"

namespace oservpp{
enum class LoginError;
}

OTSERVPP_NAMESPACE_HOOK_BEGIN

struct AccountLoginSucceedResult{
	typedef std::tuple<
			int64_t,
			std::string,
			std::vector<std::tuple<std::string, std::string, uint32_t, uint16_t>>,
			int
	> type;

	enum{ MotdId=0, Motd, CharacterList, PremiumEnd };
	enum{ CharacterName=0, CharacterWorld, CharacterIp, CharacterPort };
};

/*! Called from AccountLoginProtocol whenever a login attempt succeeds.
 * \li The first parameter is the client's IP address expressed as a string.
 * \li The second parameter is the client's granted-access-to account
 * The return value expected is self-explanatory, see the enums in AccountLoginSucceedResult.
 */
OTSERVPP_DEF_HOOK(AccountLoginSucceed,
		AccountLoginSuccedResult::type(const std::string&, const AccountPtr&));

/*! Called from AccountLoginProtocol whenever a login attempt fails.
 * \li The first parameter is the error that prevents the login
 * \li The second parameter is the client's IP address expressed as a string
 * The return value should be an string representing the error.
 */
OTSERVPP_DEF_HOOK(AccountLoginError,
		std::string(LoginError, const std::string&));

OTSERVPP_NAMESPACE_HOOK_END

#endif // OTSERVPP_HOOK_LOGINPROTOCOLHOOKS_HPP_
