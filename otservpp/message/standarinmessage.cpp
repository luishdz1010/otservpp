#include "standardinmessage.h"
#include <boost/lexical_cast.hpp>

namespace otservpp{

StandardInMessage::StandardInMessage()
{}

boost::asio::mutable_buffers_1 StandardInMessage::parseHeaderAndGetBodyBuffer()
{
	//int bodySize = getU16();
	int bodySize = boost::lexical_cast<int>((char)getByte());

	// 5 = checksumsize + 1 byte smallest packet, TODO if this correct?
	if(bodySize < 5 || bodySize > STANDARD_IN_MESSAGE_MAX_BODY_SIZE)
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



