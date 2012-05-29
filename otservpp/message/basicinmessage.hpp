#ifndef OTSERVPP_BASICINMESSAGE_HPP_
#define OTSERVPP_BASICINMESSAGE_HPP_

#include <array>
#include "../networkdcl.hpp"

namespace otservpp {

/// Simple in message for use in advanced message classes
template <std::size_t HEADER_SIZE, std::size_t MAX_BODY_SIZE>
class BasicInMessage {
public:
	typedef std::array<uint8_t, HEADER_SIZE+MAX_BODY_SIZE> Buffer;

	BasicInMessage() :
		pos(0)
	{}

	BasicInMessage(BasicInMessage&&) = default;

	boost::asio::mutable_buffers_1  getHeaderBuffer()
	{
		return boost::asio::buffer(buffer.data(), HEADER_SIZE);
	}

	boost::asio::mutable_buffers_1& getBodyBuffer(std::size_t bodySize)
	{
		assert(bodySize < MAX_BODY_SIZE);
		return boost::asio::buffer(buffer.data()+HEADER_SIZE, bodySize);
	}

	uint8_t getByte()
	{
		assert(pos+1 <= buffer.size());
		return buffer[pos++];
	}

	uint16_t getU16()
	{
		return get<uint16_t>();
	}

	uint32_t getU32()
	{
		return get<uint32_t>();
	}

	uint64_t getU64(){
		return get<uint64_t>();
	}

	void skipBytes(std::size_t b)
	{
		assert(pos+b <= buffer.size());
		pos += b;
	}

	BasicInMessage(BasicInMessage&) = delete;
	void operator=(BasicInMessage&) = delete;

protected:
	~BasicInMessage() = default;

	template <class T>
	typename std::enable_if<std::is_arithmetic<T>::value, T>::type
	get()
	{
		assert(pos+sizeof(T) <= buffer.size());
		return *(T*)(buffer.data() + ((pos+=sizeof(T))-sizeof(T)));
	}

	uint8_t* getInternalBuffer()
	{
		return buffer.data();
	}

private:
	std::size_t pos;
	Buffer buffer;
};

} /* namespace otservpp */

#endif // OTSERVPP_BASICINMESSAGE_HPP_
