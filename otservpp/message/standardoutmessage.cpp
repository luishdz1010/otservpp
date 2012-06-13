#include "standardoutmessage.h"

namespace otservpp {

void StandardOutMessage::addHeader()
{
	addPrefix((uint16_t)getSize());
}

void StandardOutMessage::xteaEncrypt(const crypto::Xtea& xtea)
{
	addPadding(8 - getSize()%8, 0x33);
	xtea.encrypt(getBufferAs<uint32_t>(), getSize());
}

void StandardOutMessage::encode()
{
	addPrefix(crypto::adler32(getBufferAs<uint8_t>(), getSize()));
	addPrefix(getSize());
}

} /* namespace otservpp */
