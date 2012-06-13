#include "standardinmessage.h"
#include <boost/lexical_cast.hpp>

namespace otservpp{

StandardInMessage::StandardInMessage()
{}

boost::asio::mutable_buffers_1 StandardInMessage::parseHeaderAndGetBodyBuffer()
{
	auto bodySize = getU16();

	// 8 = adler + 1 xtea frame smallest packet
	if(bodySize < 8 || bodySize > STANDARD_IN_MESSAGE_MAX_BODY_SIZE)
		throw PacketException();

	setRemainingSize(bodySize);
	return getBodyBuffer();
}

std::string StandardInMessage::getString()
{
	return getStrChunck(getU16());
}

void StandardInMessage::doChecksum()
{
	auto givenChecksum = getU32();
	auto size = getRemainingSize();

	if(givenChecksum != crypto::adler32(peekRawChunckAs<uint8_t>(size), size))
		throw PacketException();
}

crypto::Xtea StandardInMessage::getXtea()
{
	return {getU32(), getU32(), getU32(), getU32()};
}

void StandardInMessage::xteaDecrypt(const crypto::Xtea& xtea)
{
	auto size = getRemainingSize();

	if(size%8 != 0)
		throw PacketException();

	xtea.decrypt(peekRawChunckAs<uint32_t>(size), size);

	setRemainingSize(getU16());
}

}
