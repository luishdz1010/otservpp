#ifndef OTSERVPP_STANDARDINMESSAGE_H_
#define OTSERVPP_STANDARDINMESSAGE_H_

#include "basicinmessage.hpp"
#include "../crypto.h"

namespace otservpp {

/// Constants for buffer sizes
// TODO 15340 is taken from otserv, is this an arbitrary constant?
enum{
	STANDARD_IN_MESSAGE_HEADER_SIZE = 2,
	STANDARD_IN_MESSAGE_MAX_BODY_SIZE = 15340
};

/// Standard incoming tibia packet.
class StandardInMessage :
	public BasicInMessage<STANDARD_IN_MESSAGE_HEADER_SIZE, STANDARD_IN_MESSAGE_MAX_BODY_SIZE>{
public:
	StandardInMessage();

	/*! Called by Connection when the header buffer is filled
	 * Returns the message's body buffer
	 */
	boost::asio::mutable_buffers_1 parseHeaderAndGetBodyBuffer();

	/*! Called by Connection when the body buffer is filled
	 * Here we perform a basic adler32 chksum
	 */
	void parseBody();

	/// Extracts a string
	std::string getString();

	/// Extracts the client's version
	uint16_t getClientVersion();

	/// Extracts the XTEA key
	crypto::Xtea getXtea();

	/// Decrypts the message using the previously Xtea object returned by rsaDecrypt
	void xteaDecrypt(crypto::Xtea& xtea);

	/// Decrypts the message using the given RSA object, calls handler(true) on success or
	/// handler(false) on failure.
	template <class Handler>
	void rsaDecrypt(crypto::Rsa& rsa, Handler&& handler)
	{
		rsa.decrypt(peekRawChunck(128), 128, [this, handler]
		(boost::system::error_condition& e, int newSize){
			if(!e){
				setRemainingSize(newSize);
				handler(getByte() == 0);
			} else{
				handler(false);
			}
		});
	}
};

} /* namespace otservpp */

#endif // OTSERVPP_STANDARDINMESSAGE_H_
