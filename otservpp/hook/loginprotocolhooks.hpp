#ifndef OTSERVPP_HOOK_LOGINPROTOCOLHOOKS_HPP_
#define OTSERVPP_HOOK_LOGINPROTOCOLHOOKS_HPP_

#include <string>
#include <vector>
#include "hook.hpp"

namespace oservpp{
enum class AccountLoginError;
}

OTSERVPP_NAMESPACE_HOOK_BEGIN

struct AccountPtr{};

struct CharacterListItem{
	typedef std::tuple<std::string, std::string, int32_t, int16_t> type;
	enum{Name=0, World, Ip, Port};
};

struct AccountLoginSuccedResult{
	typedef std::tuple<std::string, std::vector<CharacterListItem::type>, int> type;
	enum{Motd=0, CharacterList, PremiumEnd};
};

typedef Hook<
		AccountLoginSuccedResult::type
		//(boost::asio::ip::address_v4, AccountPtr)
		(int, std::string)
> AccountLoginSucced;

typedef Hook<
		std::string
		(AccountLoginError, int32_t, AccountPtr)
> AccountLoginFailed;


OTSERVPP_NAMESPACE_HOOK_END

#endif // OTSERVPP_HOOK_LOGINPROTOCOLHOOKS_HPP_
