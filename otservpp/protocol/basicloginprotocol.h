#ifndef OTSERVPP_BASICLOGINPROTOCOL_H_
#define OTSERVPP_BASICLOGINPROTOCOL_H_

#include "standardprotocol.hpp"
#include <boost/algorithm/string/trim.hpp>

namespace otservpp {

enum class LoginError{
	NoError = 0,
	BadClientVersion,
	IpBanned,
	IpDisabled,
	EmptyCredentials,
	BadCredentials,
	GameNotRunning
};

struct BasicLoginProtocolData{
	crypto::Rsa& rsa;
	io::Account& accountIo;
};

/*! Base class for AccountLoginProtocol and GameLoginProtocol
 * BasicLoginProtocol contains some common methods for the login protocol classes. DRY
 */
template <class Protocol>
class BasicLoginProtocol : public StandardProtocol<Protocol>{
public:
	BasicLoginProtocol(const ConnectionPtrType& conn, BasicLoginProtocolData& data) :
		StandardProtocol(conn),
		impl(data)
	{}

protected:
	typedef StandardProtocol<Protocol> StdProtocol;
	using StdProtocol::handlePacketException;
	using StdProtocol::droppingLogInfo;
	using StdProtocol::setXtea;

	bool validateIpAndVersion(StandardInMessage& msg, LoginError& e)
	{
		// TODO validate ip

		msg.skipBytes(3); // protocol-id/OS/0x01
		auto version = msg.getU16();

		// TODO validate version

		return true;
	}

	template <class Handler>
	void decryptMessageAndSetXta(StandardInMessage& msg, Handler&& handler)
	{
		msg.rsaDecrypt(impl.rsa, wrapHandler(
		[this, handler](bool success){
			if(success){
				setXta(msg->getXtea());
				handler();
			} else {
				LOG(INFO) << "RSA decryption error" << droppingLogInfo();
				connection->stop();
			}
		}));
	}

	template <class Handler>
	void loadAccount(const std::string& accName, const std::string& password, Handler&& handler)
	{
		if(checkCredentials<AccountPtr>(accName, password, handler)){
			impl.accountIo.loadAccount(wrapHandler(
			[handler](const AccountPtr& acc){
				if(acc)
					handler(LoginError::NoError, acc);
				else
					handler(LoginError::BadCredentials, acc);
			}));
		}
	}

	template <class Handler>
	void loadAccountId(const std::string& accName, const std::string& password, Handler&& handler)
	{
		if(checkCredentials<int64_t>(accName, password, handler)){
			impl.accountIo.loadAccountId(wrapHandler([](int64_t accId){
				if(accId > 0)
					handler(LoginError::NoError, accId);
				else
					handler(LoginError::BadCredentials, accId);
			}));
		}
	}

private:
	template <class Arg, class Handler>
	bool checkCredentials(std::string accName, std::string password, Handler& handler)
	{
		boost::algorithm::trim(accName);
		boost::algorithm::trim(password);

		if(accName.empty() || password.empty()){
			wrapHandler(std::move(handler))(LoginError::EmptyCredentials, Arg());
			return false;
		}

		return true;
	}

	LoginSharedData& impl;
};

} /* namespace otservpp */

#endif // OTSERVPP_BASICLOGINPROTOCOL_H_
