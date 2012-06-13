#ifndef OTSERVPP_LOGINPROTOCOL_H_
#define OTSERVPP_LOGINPROTOCOL_H_

#include "basicloginprotocol.h"
#include "../hook/loginprotocolhooks.hpp"

namespace otservpp {

struct AccountLoginProtocolData{
	hook::AccountLoginSucceed succeedHook;
	hook::AccountLoginError errorHook;
};

/// The standard tibia login protocol
class AccountLoginProtocol :
	public BasicLoginProtocol<AccountLoginProtocol>,
	public std::enable_shared_from_this<AccountLoginProtocol>
{
public:
	typedef hook::AccountLoginSucceedResult SucceedHookResult;

	/// Outgoing packet headers
	enum : uint8_t{
		ErrorByte = 0x0A,
		SucceedByte = 0x14,
		CharacterListByte = 0x64,
	};

	/// Succeed Hook tuple indexes
	enum{
		MotdId = SucceedHookResult::MotdId,
		Motd = SucceedHookResult::Motd,
		CharacterList = SucceedHookResult::CharacterList,
		PremiumEnd = SucceedHookResult::PremiumEnd,

		CharacterName = SucceedHookResult::CharacterName,
		CharacterWorld = SucceedHookResult::CharacterWorld,
		CharacterIp = SucceedHookResult::CharacterIp,
		CharacterPort = SucceedHookResult::CharacterPort
	};

	AccountLoginProtocol(const ConnectionPtrType& conn,
			BasicLoginProtocolData& d1,
			AccountLoginProtocolData& d2) :
		BasicLoginProtocol(conn, d1),
		impl(d2)
	{}

	static const char* getName()
	{
		return "account login protocol";
	}

	void onFirstMessage(StandardInMessage& msg);

private:
	/// Sends the character list provided by hook::AccountLoginSucceed after a successful
	/// login attempt
	void sendCharacterList(AccountPtr& account);

	/// Rejects the login attempt with an error message returned by hook::AccountLoginError
	void closeWithError(LoginError error);

	AccountLoginProtocolData& impl;
};

} /* namespace otservpp */

#endif // OTSERVPP_LOGINPROTOCOL_H_
