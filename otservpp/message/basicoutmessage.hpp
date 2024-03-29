#ifndef OTSERVPP_BASICOUTMESSAGE_HPP_
#define OTSERVPP_BASICOUTMESSAGE_HPP_

#include <vector>
#include <string>
#include <cstring>
#include <cassert>
#include <boost/asio/buffer.hpp>

namespace otservpp{

template <class Derived, int MAX_PREFIX_SIZE, int MAX_BUFFER_SIZE>
class BasicOutMessage{
public:
	typedef std::vector<uint8_t> BufferType;

	BasicOutMessage(unsigned int capacity) :
		buffer(MAX_PREFIX_SIZE + capacity),
		prefixPos(MAX_PREFIX_SIZE)
	{
		buffer.resize(MAX_PREFIX_SIZE);
	}

	BasicOutMessage(const BasicOutMessage&) = default;
	BasicOutMessage(BasicOutMessage&&) = default;

	boost::asio::const_buffers_1 getBuffer() const
	{
		return boost::asio::buffer(buffer.data()+prefixPos, buffer.size()-prefixPos);
	}

	std::size_t getSize()
	{
		assert((int)buffer.size() >= prefixPos);
		return buffer.size()-prefixPos;
	}

	template <class T>
	typename std::enable_if<std::is_integral<T>::value>::type
	addPrefix(T v)
	{
		assert(prefixPos - sizeof(T) >= 0);
		*reinterpret_cast<T*>(&buffer[prefixPos-=sizeof(T)]) = v;
	}

	void addByte(uint8_t v)
	{
		buffer.push_back(v);
	}

	void addU16(uint16_t v)
	{
		add(v);
	}

	void addU32(uint32_t v)
	{
		add(v);
	}

	void addU64(uint64_t v)
	{
		add(v);
	}

	void addFloat(float v)
	{
		add(v);
	}

	void addDouble(double v)
	{
		add(v);
	}

	void addString(const std::string& str)
	{
		add(str);
	}

	void addString(const char* str)
	{
		add(str);
	}

	void addString(const char* str, uint16_t strSize)
	{
		add(str, strSize);
	}

	void addRaw(const char* raw)
	{
		addRaw(raw, strlen(raw));
	}

	void addRaw(const char* raw, std::size_t size)
	{
		buffer.insert(buffer.end(), (const uint8_t*)raw, (const uint8_t*)raw+size);
	}

	Derived& operator<<(uint8_t v)
	{
		add(v);
		return derived();
	}

	Derived& operator<<(uint16_t v)
	{
		add(v);
		return derived();
	}

	Derived& operator<<(uint32_t v)
	{
		add(v);
		return derived();
	}

	Derived& operator<<(uint64_t v)
	{
		add(v);
		return derived();
	}

	Derived& operator<<(const std::string& v)
	{
		addString(v);
		return derived();
	}

	Derived& operator<<(float v)
	{
		add(v);
		return derived();
	}

	Derived& operator<<(double v)
	{
		add(v);
		return derived();
	}

protected:
	~BasicOutMessage() = default;

	template <class T>
	typename std::enable_if<std::is_arithmetic<T>::value>::type
	add(T value)
	{
		auto pos = buffer.size();
		buffer.resize(pos+sizeof(T));
		*reinterpret_cast<T*>(&buffer[pos]) = value;
	}

	void add(const std::string& str)
	{
		auto strSize = (uint16_t)str.size();
		buffer.reserve(2+strSize);
		add(strSize);
		buffer.insert(buffer.end(), str.cbegin(), str.cend());
	}

	void add(const char* str, uint16_t strSize)
	{
		buffer.reserve(2+strSize);
		add(strSize);
		buffer.insert(buffer.end(), str, str+strSize);
	}

	void add(const char* str)
	{
		add(str, strlen(str));
	}

	void addPadding(std::size_t count, uint8_t value)
	{
		buffer.resize(buffer.size()+count, value);
	}

	template <class T>
	typename std::enable_if<std::is_integral<T>::value, T*>::type
	getBufferAs()
	{
		return reinterpret_cast<T*>(&buffer[prefixPos]);
	}

private:
	Derived& derived()
	{
		return static_cast<Derived&>(*this);
	}

	BufferType buffer;
	int prefixPos;
};

} /* namespace otservpp */

#endif // OTSERVPP_BASICOUTMESSAGE_HPP_
