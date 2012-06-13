#ifndef OTSERVPP_BASICINMESSAGE_HPP_
#define OTSERVPP_BASICINMESSAGE_HPP_

#include <array>
#include "../networkdcl.hpp"

namespace otservpp {

/// Indicates a failure while trying to overrun the message's buffer
class PacketException : public std::exception{};

/*! Simple in-message for use in advanced message classes
 * The buffer is statically allocated, since most protocols use small buffers size this is OK.
 */
template <unsigned int HEADER_SIZE, unsigned int MAX_BODY_SIZE>
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
	unsigned int getSize() const
	{
		return size;
	}

	/// Returns the remaining size of the message (size - readpos)
	unsigned int getRemainingSize() const
	{
		assert(size >= pos);
		return size-pos;
	}

	/// Returns true if getRemainingSize() >= bytesRequired
	bool isAvailable(unsigned int bytesRequired) const
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

	uint8_t* getRawChunck(unsigned int bytes)
	{
		return getRawChunckAs<uint8_t>(bytes);
	}

	std::string getStrChunck(unsigned int bytes)
	{
		return std::string(getRawChunckAs<char>(bytes), bytes);
	}

	void skipBytes(unsigned int delta)
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
	T* getRawChunckAs(unsigned int bytes)
	{
		movePos(bytes);
		return reinterpret_cast<T*>(buffer.data() + (pos-bytes));
	}

	void movePos(unsigned int bytes)
	{
		if((pos+=bytes) > size)
			throw PacketException();
	}

	template<class T>
	typename std::enable_if<std::is_integral<T>::value, T*>::type
	peekRawChunckAs(unsigned int bytes)
	{
		if(pos+bytes > size)
			throw PacketException();
		else
			return reinterpret_cast<T*>(buffer.data() + pos);
	}

	void setRemainingSize(unsigned int bytes)
	{
		assert(pos+bytes <= MAX_BODY_SIZE);
		size = pos+bytes;
	}

private:
	Buffer buffer;
	unsigned int pos;
	unsigned int size;
};

} /* namespace otservpp */

#endif // OTSERVPP_BASICINMESSAGE_HPP_
