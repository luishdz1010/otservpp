#ifndef OTSERVPP_STANDARDOUTMESSAGE_H_
#define OTSERVPP_STANDARDOUTMESSAGE_H_

#include "basicoutmessage.hpp"

namespace otservpp {

enum : uint8_t{
	/// RawPacketLen(2) + Adler32(4) + DecryptedPacketLen(2)
	STANDARD_OUT_MESSAGE_PREFIX_SIZE = 8
};

class StandardOutMessage :
	public BasicOutMessage<StandardOutMessage, STANDARD_OUT_MESSAGE_PREFIX_SIZE>
{
public:
	///  Created a StandardOutMessage with a reasonable initial buffer
	StandardOutMessage() :
		BasicOutMessage(1024)
	{}

	/// Shorthand for \code StandardOutMessage msg; msg.addByte(packetType); \endcode
	StandardOutMessage(uint8_t packetType) :
		BasicOutMessage(1024)
	{
		add(packetType);
	}

	/// Creates a StandarOutMessage with a minimal buffer required to store the passed args
	template <class Arg, class... Args>
	StandardOutMessage(uint8_t packetType, Arg&& arg, Args&&... args) :
		BasicOutMessage(1 + calculateSize(args...))
	{
		add(packetType);
		add(std::forward<Arg>(arg));
		add(std::forward<Args>(args))...;
	}

	/// Prepends the packet size information, after calling this function no more data should be
	/// added to the message
	void addHeader();

	/// Encrypts the message using the given XTEA structure
	void xteaEncrypt(const crypto::Xtea& xtea);

	/// Called by otservpp::Connection before actually sending the packet, we do some defered
	/// packaging here
	void encode();

private:
	template <class... Args>
	uint16_t calculateSize(Args&... args)
	{
		uint16_t size;
		calcSize(size, args...);
		return size;
	}

	template <class T>
	void calcSize(uint16_t& size, T& arg)
	{
		size += calcOneSize(arg);
	}

	template <class T, class... Others>
	void calcSize(uint16_t& size, T& arg, Others&... others)
	{
		size += calcOneSize(arg);
		calcSize(size, others...);
	}

	template <class T>
	typename std::enable_if<std::is_integral<T>::value>::type
	calcOneSize(T& arg)
	{
		size += sizeof(T);
	}

	void calcOneSize(std::string& str)
	{
		return str.size();
	}
};

} /* namespace otservpp */

#endif // OTSERVPP_STANDARDOUTMESSAGE_H_
