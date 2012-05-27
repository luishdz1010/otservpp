#include "loginprotocol.h"
#include <cassert>

using namespace otservpp;

LoginProtocol::LoginProtocol(LoginSharedData& data) :
	p(data)
{}

StandardInMessage LoginProtocol::createIncomingMessage()
{
	return {p.checksumMode};
}

void LoginProtocol::handleFirstMessage(StandardInMessage& msg)
{
	auto& conn = getConnection()->stopReceiving();

	p.getPeerState(conn->getPeerAddress(), [=, &msg](PeerState s){
		switch(s){
		case PeerState::Ok:
			break;
		case PeerState::Disabled:
			return closeWithError(AccountLoginError::IpDisabled);
		case PeerState::Banned:
			return closeWithError(AccountLoginError::IpBanned);
		}

		msg.skipBytes(1); // protocol id

		if(p.validVersion(msg.getClientVersion()))
			return closeWithError(AccountLoginError::BadClientVersion);

		msg.decryptRSA();

		std::string accName = msg.getString();

		if(accName.empty())
			return closeWithError(AccountLoginError::BadAccountName);

		p.loadAccount(accName, [this, conn, &msg](const AccountPtr& account){
			if(!account || !account->passwordsMatch(msg.getString()))
				return closeWithError(AccountLoginError::BadPassword);

			StandarOutMessage out;
			auto response = p.onAccountLogin(account, ip);

			out.addByte(Motd);
			out << std::get<OnAccountLogin::Motd>(response);

			out.addByte(CharacterList);
			for(auto& it : std::get<OnAccountLogin::CharacterList>(response)){
				out << std::get<OnAccountLogin::CharacterName>(it)
					<< std::get<OnAccountLogin::CharacterWorld>(it)
					<< std::get<OnAccountLogin::ServerIp>(it)
					<< std::get<OnAccountLogin::ServerPort>(it);
			}

			getConnection()->sendAndClose(out);
		});
	});
}

void LoginProtocol::closeWithError(AccountLoginError error, const AccountPtr& acc)
{
	p.dispatcher.callLater([=]() mutable {
		StandarOutMessage msg;
		auto r = p.onAccountLoginError(getConnection()->getPeerAddress().to_string(), error, acc);

		msg.addByte(Error);
		msg << std::get<OnAccountLoginError::Message>(r);

		getConnection()->sendAndClose(msg);
	});
}
