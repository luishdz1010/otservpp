#include "standardinmessage.h"

namespace otservpp{

StandardInMessage::StandardInMessage()
{}

boost::asio::mutable_buffers_1 StandardInMessage::parseHeaderAndGetBodyBuffer()
{
	int bodySize = getU16();

	// 5 = checksumsize + 1 byte smallest packet, TODO if this correct?
	if(bodySize < 5 || bodySize > STANDAR_IN_MESSAGE_BODY_SIZE)
		throw std::runtime_error("invalid packet length size, dropping");

	return getBodyBuffer(bodySize);
}

/*
void StandardInMessage::parseBody()
{
	if(checksumMode == ChecksumMode::Check){
		//TODO adler32 check
		skipBytes(4);
	} else{
		skipBytes(4);
	}
}

uint16_t StandardInMessage::getClientVersion()
{
	skipBytes(2); // OS && 0x01
	auto version = getU16();
	skipBytes(12); // file checksum
	return version;
}*/

}



