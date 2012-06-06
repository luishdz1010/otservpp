#include "accountloginprotocol.h"
#include "../hook/loginprotocolhooks.hpp"


namespace otservpp {

using namespace hook;

void AccountLoginProtocol::handleFirstMessage(StandardInMessage& msg)
{
	connection->stopReceiving();
}
	/*msg.skipBytes(1); // protocolid
	auto version = msg.getClientVersion();
	msg.skipBytes(12);

	msg.rsaDecrypt(rsa, [this, &msg](bool success){
		if(success){
			auto xtea = msg.getXtea();
			auto user = msg.getString();
			auto pass = msg.getString();
			int x = 12;
		} else{
			throw std::runtime_error("error_");
		}
	});


	p.getPeerState(connection->getPeerAddress(), [this, &msg](PeerState s){
		switch(s){
		case PeerState::Ok:
			break;
		case PeerState::Banned:
			return closeWithError(AccountLoginError::IpBanned);
		case PeerState::Disabled:
			return closeWithError(AccountLoginError::IpDisabled);
		}

		msg.skipBytes(1); // protocol id

		if(p.validVersion(getClientVersion(msg)))
			return closeWithError(AccountLoginError::BadClientVersion);

		decryptRSA(msg); // ignore xtea key?

		auto accName = msg.getString();

		if(accName.empty())
			return closeWithError(AccountLoginError::BadAccountName);

		p.loadAccount(accName, [this, &msg](const AccountPtr& account){
			if(!account || !account->passwordsMatch(msg.getString()))
				return closeWithError(AccountLoginError::BadPassword);

			StandarOutMessage out;
			auto response = p.onAccountLogin(ip, account);

			out.addByte(Motd);
			out << std::get<AccountLoginSuccedResult::Motd>(response);

			out.addByte(CharacterList);
			for(auto& it : std::get<AccountLoginSuccedResult::CharacterList>(response)){
				out << std::get<CharacterListItem::Name>(it)
					<< std::get<CharacterListItem::World>(it)
					<< std::get<CharacterListItem::Ip>(it)
					<< std::get<CharacterListItem::Port>(it);
			}

			connection->sendAndStop(out);
		});
	});
}

void LoginProtocol::closeWithError(AccountLoginError error, const AccountPtr& acc)
{
	p.dispatcher.callLater([=]() {
		StandarOutMessage msg;
		auto r = p.onAccountLoginError(getConnection()->getPeerAddress().to_string(), error, acc);

		msg.addByte(Error);
		msg << std::get<OnAccountLoginError::Message>(r);

		getConnection()->sendAndClose(msg);
	});
}*/

}
