#ifndef OTSERVPP_BASICINMESSAGE_HPP_
#define OTSERVPP_BASICINMESSAGE_HPP_

#include <array>
#include "../networkdcl.hpp"

namespace otservpp {

/*! Simple in-message for use in advanced message classes
 * The buffer is statically allocated, since most protocols use small buffers size this is OK.
 */
template <uint HEADER_SIZE, uint MAX_BODY_SIZE>
class BasicInMessage {
public:
	typedef std::array<uint8_t, HEADER_SIZE+MAX_BODY_SIZE> Buffer;

	BasicInMessage() :
		pos(0),
		size(HEADER_SIZE)
	{}

	BasicInMessage(BasicInMessage&&) = default;

	boost::asio::mutable_buffers_1  getHeaderBuffer()
	{
		return boost::asio::buffer(buffer.data(), HEADER_SIZE);
	}

	boost::asio::mutable_buffers_1 getBodyBuffer()
	{
		assert(size > HEADER_SIZE);
		return boost::asio::buffer(buffer.data()+HEADER_SIZE, size-HEADER_SIZE);
	}

	/// Returns the total size of the message (header + body)
	uint getSize()
	{
		return size;
	}

	/// Returns the remaining size of the message (size - readpos)
	uint getRemainingSize()
	{
		assert(size >= pos);
		return size-pos;
	}

	/// Returns true if getRemainingSize() >= bytesRequired
	bool isAvailable(uint bytesRequired)
	{
		return pos+bytesRequired <= size;
	}

	uint8_t getByte()
	{
		movePos(1);
		return buffer[pos-1];
	}

	uint16_t getU16()
	{
		return getNumber<uint16_t>();
	}

	uint32_t getU32()
	{
		return getNumber<uint32_t>();
	}

	uint64_t getU64()
	{
		return getNumber<uint64_t>();
	}

	uint8_t* getRawChunck(uint bytes)
	{
		return getRawChunckAs<uint8_t>(bytes);
	}

	std::string getStrChunck(uint bytes)
	{
		return std::string(getRawChunckAs<char>(bytes), bytes);
	}

	void skipBytes(uint delta)
	{
		movePos(delta);
	}

	BasicInMessage(BasicInMessage&) = delete;
	void operator=(BasicInMessage&) = delete;

protected:
	~BasicInMessage() = default;

	template <class T>
	typename std::enable_if<std::is_arithmetic<T>::value, T>::type
	getNumber()
	{
		return *getRawChunckAs<T>(sizeof(T));
	}

	template <class T>
	T* getRawChunckAs(uint bytes)
	{
		movePos(bytes);
		return reinterpret_cast<T*>(buffer.data() + (pos-bytes));
	}

	void movePos(uint bytes)
	{
		if((pos+=bytes) > size)
			throwBufferException();
	}

	uint8_t* peekRawChunck(uint bytes)
	{
		if(pos+bytes > size)
			throwBufferException();
		else
			return buffer.data() + pos;
	}

	void throwBufferException()
	{
		throw std::out_of_range("attempt to overrun the incoming message buffer");
	}

	void setRemainingSize(uint bytes)
	{
		assert(pos+bytes <= MAX_BODY_SIZE);
		size = pos+bytes;
	}

private:
	Buffer buffer;
	uint pos;
	uint size;
};

} /* namespace otservpp */

#endif // OTSERVPP_BASICINMESSAGE_HPP_
