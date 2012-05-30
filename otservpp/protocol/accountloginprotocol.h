#ifndef OTSERVPP_LOGINPROTOCOL_H_
#define OTSERVPP_LOGINPROTOCOL_H_

#include "standardprotocol.hpp"

namespace otservpp {

enum class AccountLoginError{
	BadClientVersion,
	IpBanned,
	IpDisabled,
	BadAccountName,
	BadPassword,
	GameNotRunning
};

struct LoginSharedData{
/*	NetworkRecord& record;
	//GameStatus& gameState;
	AccountLoader& accLoader;
	ChecksumMode& checksumMode;
	*/
};

/// The standard tibia login protocol
class AccountLoginProtocol : public StandardProtocol{
public:
	enum{
		Error = 0x0A,
		Motd = 0x14,
		CharacterList = 0x64
	};

	explicit AccountLoginProtocol(LoginSharedData& data);

	static const char* getName()
	{
		return "account login protocol";
	}

	/// Handles the login message. If the login succeed
	/// LoginSharedData::loginSuccedHook(peerAddress, account) is called to obtain the
	/// the login info
	void handleFirstMessage(StandardInMessage& msg);

private:
	/// Rejects the login attempt with an error code using the message provided from
	/// LoginSharedData::loginFailedHook(peerAddress, error code[, account])
	//void closeWithError(AccountLoginError error, const AccountPtr& acc = AccountPtr());

	LoginSharedData& p;
};

} /* namespace otservpp */

#endif // OTSERVPP_LOGINPROTOCOL_H_
