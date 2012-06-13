#include "accountloginprotocol.h"
#include <boost/lexical_cast.hpp>

namespace otservpp {

void AccountLoginProtocol::onFirstMessage(StandardInMessage& msg)
{
	connection->stopReceiving();

	LoginError e;
	if(!validateIpAndVersion(msg, e))
		return closeWithError(e);

	msg.skipBytes(12); // files checksum

	auto sthis = shared_from_this();

	decryptMessageAndSetXta(msg, [this, &msg, sthis]{
		loadAccount(msg.getString(), msg.getString(),
		[this, sthis](LoginError e, AccountPtr account){
			if(e != LoginError::NoError)
				closeWithError(e);
			else
				sendCharacterList(account);
		});
	});
}

void AccountLoginProtocol::sendCharacterList(AccountPtr& account)
{
	SucceedHookResult res;

	try{
		res = impl.succeedHook(connection->getPeerAddress()->to_string(), account);
	} catch(std::exception& e){
		LOG(ERROR) << e.message()  << "\nwhile executing " << impl.succeedHook.getName()
					<< droppingLogInfo();
		connection->stop();
		return; // here we could add some backup code, in wish list though
	}

	using namespace std;

	auto& charList = get<CharacterList>(res);
	auto size = charList.size();

	if(size > UINT8_MAX){
		LOG(ERROR) << "invalid character list size(" << size << ") returned by "
					<< impl.succeedHook.getName() << droppingLogInfo();
		connection->stop();

	} else {
		StandardOutMessage out{SucceedByte};

		out << boost::lexical_cast<string>(get<MotdId>(res)) + "\n" + get<Motd>(res);

		out.addByte(CharacterListByte);
		out.addByte((uint8_t)size);
		for(auto& it : charList){
			out << get<CharacterName>(it)
				<< get<CharacterWorld>(it)
				<< get<CharacterIp>(it)
				<< get<CharacterPort>(it);
		}

		sendAndStop(out);
	}
}

void AccountLoginProtocol::closeWithError(LoginError error)
{
	std::string errorString;

	try{
		errorString = impl.errorHook(error, connection->getPeerAddress().to_string());
	} catch(std::exception& e){
		LOG(ERROR) << e.message() << "\nwhile executing " << impl.errorHook.getName()
					<< droppingLogInfo();
		connection->stop();
		return;
	}

	sendAndStop({ErrorByte, errorString});
}

}
