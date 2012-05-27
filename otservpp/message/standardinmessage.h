#ifndef OTSERVPP_STANDARDINMESSAGE_H_
#define OTSERVPP_STANDARDINMESSAGE_H_

#include "basicinmessage.hpp"

namespace otservpp {

/// Whether to skip or do check the checksum
enum class ChecksumMode{
	Check,
	Skip
};

/// Constants for buffer sizes
// TODO 15340 is taken from otserv project, is this an arbitrary constant?
enum{
	STANDAR_IN_MESSAGE_HEADER_SIZE = 2,
	STANDAR_IN_MESSAGE_MAX_BODY_SIZE = 15340
};

/// Standard incoming tibia packet.
class StandardInMessage :
	public BasicInMessage<STANDAR_IN_MESSAGE_HEADER_SIZE, STANDAR_IN_MESSAGE_MAX_BODY_SIZE>{
public:
	typedef int XteaKey;

	/// Constructs a new standar in msg, the given mode should outlive this object
	/// rationale: the mode might change at runtime, so all the standar in msgs should
	/// share the same mode
	explicit StandardInMessage(ChecksumMode& mode);

	/*! Called by Connection when the header buffer is filled
	 * Returns the message's body buffer
	 */
	boost::asio::mutable_buffers_1 parseHeaderAndGetBodyBuffer();

	/*! Called by Connection when the body buffer is filled
	 * After doing or skipping the checksum, the Connection will dispatch the packet to its
	 * respective protocol.
	 */
	void parseBody();

	/// Extracts an string from the message
	std::string getString();

	/// Extracts the peer's client version
	uint16_t getClientVersion();

	/// Decrypts the message using the predefined RSA key and returns the extracted XTEA key
	XteaKey decryptRSA(Callback&& callback);

	/// Decrypts the message using the given XTEA key
	void decryptXTEA(const XteaKey& key);

private:
	ChecksumMode& checksumMode;
};

} /* namespace otservpp */

#endif // OTSERVPP_STANDARDINMESSAGE_H_
