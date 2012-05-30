#ifndef OTSERVPP_STANDARDINMESSAGE_H_
#define OTSERVPP_STANDARDINMESSAGE_H_

#include "basicinmessage.hpp"

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
	 * TODO remove me?
	 */
	void parseBody();

	std::string getString();
};

} /* namespace otservpp */

#endif // OTSERVPP_STANDARDINMESSAGE_H_
