#ifndef OTSERVPP_LOGINPROTOCOL_H_
#define OTSERVPP_LOGINPROTOCOL_H_

#include "basic.hpp"
#include "../message/standardinmessage.h"
#include "traits.hpp"

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
class LoginProtocol : public BasicProtocol<LoginProtocol> {
public:
	enum{
		Error = 0x0A,
		Motd = 0x14,
		CharacterList = 0x64
	};

	explicit LoginProtocol(LoginSharedData& data);

	/// Returns a StandarInMessage with the same ChecksumMode as in the underlying
	/// LoginSharedData::checksumMode
	StandardInMessage createIncomingMessage();

	void handleFirstMessage(StandardInMessage& msg);

private:
	void closeWithError(AccountLoginError errorCode);

	LoginSharedData& p;
};

} /* namespace otservpp */

#endif // OTSERVPP_LOGINPROTOCOL_H_
