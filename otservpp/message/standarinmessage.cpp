#include "standardinmessage.h"
#include <boost/lexical_cast.hpp>

namespace otservpp{

StandardInMessage::StandardInMessage()
{}

boost::asio::mutable_buffers_1 StandardInMessage::parseHeaderAndGetBodyBuffer()
{
	auto bodySize = getU16();

	// 8 = adler + 1 xtea frame smallest packet, TODO if this correct?
	if(bodySize < 8 || bodySize > STANDARD_IN_MESSAGE_MAX_BODY_SIZE)
		throw std::runtime_error("invalid packet size, dropping");

	setRemainingSize(bodySize);
	return getBodyBuffer();
}

std::string StandardInMessage::getString()
{
	return getStrChunck(getU16());
}

void StandardInMessage::parseBody()
{
	skipBytes(4); // TODO implement adler32
}

uint16_t StandardInMessage::getClientVersion()
{
	skipBytes(2); // OS && 0x01
	return getU16();
}

crypto::Xtea StandardInMessage::rsaDecrypt(crypto::Rsa& rsa)
{
	setRemainingSize((uint)rsa.decrypt(peekRawChunck(128), 128));

	if(getByte() != 0)
		throw std::runtime_error("rsa decryption failed");

	return {getU32(), getU32(), getU32(), getU32()};
}

void StandardInMessage::xteaDecrypt(crypto::Xtea& xtea)
{
	auto size = getRemainingSize();
	xtea.decrypt(getRawChunckAs<uint32_t>(size), size);
	setRemainingSize(getU16());
}

}
