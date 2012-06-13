#include <gtest/gtest.h>
#include <algorithm>
#include <boost/asio/buffer.hpp>
#include "otservpp/message/basicoutmessage.hpp"

#define DCLT(Sufix, Type) \
struct Sufix{ \
	typedef Type ValueType;\
	template <class Msg, class T> \
	static void addFunc(Msg& msg, T& v){ msg.add##Sufix(v); } \
};

using otservpp::BasicOutMessage;
using boost::asio::buffer_size;
using boost::asio::buffer_cast;

namespace{
	DCLT(Byte, uint8_t);
	DCLT(U16, uint16_t);
	DCLT(U32, uint32_t);
	DCLT(U64, uint64_t);
	//DCLT(Float, float);
	//DCLT(Double, double);

	typedef ::testing::Types<Byte, U16, U32, U64> TestTypes;
}

template <class T>
class BasicOutMessageTest : public ::testing::Test{
public:
	typedef typename T::ValueType const ValueType;

	struct TestMessage : public BasicOutMessage<TestMessage, 100, 10000>{
		TestMessage() : BasicOutMessage<TestMessage, 100, 10000>(10000){}
	};

	BasicOutMessageTest() {}

	void checkSize(int expected)
	{
		ASSERT_EQ(expected, msg.getSize());
		ASSERT_EQ(expected, buffer_size(msg.getBuffer()));
	}

	ValueType* getBuffer()
	{
		return buffer_cast<ValueType*>(msg.getBuffer());
	}

	static ValueType rand()
	{
		return std::rand();
	}

	TestMessage msg;
	ValueType value = rand();
	const int valueSize = sizeof(ValueType);
};

TYPED_TEST_CASE(BasicOutMessageTest, TestTypes);

TYPED_TEST(BasicOutMessageTest, IsEmptyOnStart){
	this->checkSize(0);
}

TYPED_TEST(BasicOutMessageTest, IncrementsSizeWhenAddingValue){
	TypeParam::addFunc(this->msg, this->value);
	this->checkSize(this->valueSize);
}

TYPED_TEST(BasicOutMessageTest, AddsValueWithLeftShiftOperator){
	this->msg << this->value;
	this->checkSize(this->valueSize);
}

TYPED_TEST(BasicOutMessageTest, IncrementsSizeWhenAddingPrefix){
	this->msg.addPrefix(this->value);
	this->checkSize(this->valueSize);
}

TYPED_TEST(BasicOutMessageTest, InsertsIntoBufferAddedValues){
	this->msg << this->value;
	ASSERT_EQ(this->value, *this->getBuffer());
}

TYPED_TEST(BasicOutMessageTest, PrependedPrefixesToBuffer){
	decltype(this->value) value2 = this->value + this->valueSize;
	this->msg.addPrefix(this->value);
	this->msg.addPrefix(value2);

	this->checkSize(this->valueSize*2);
	ASSERT_EQ(value2, *this->getBuffer());
	ASSERT_EQ(this->value, *(this->getBuffer()+1));
}

TYPED_TEST(BasicOutMessageTest, AddsRawDataCorrectly){
	std::vector<typename TypeParam::ValueType> values(std::min((int)this->value, 10000));
	std::generate(values.begin(), values.end(), this->rand);

	this->msg.addRaw((const char*)values.data(), values.size()*this->valueSize);

	this->checkSize(values.size()*this->valueSize);

	for(unsigned i = 0; i != values.size(); ++i)
		ASSERT_EQ(values[i], this->getBuffer()[i]) << "on position " << i << " of "
														<< values.size();
}

TYPED_TEST(BasicOutMessageTest, AddsDataAfterPrefixes){
	decltype(this->value) value2 = this->value + this->valueSize;
	decltype(this->value) value3 = value2/8;

	this->msg << this->value;
	this->msg.addPrefix(value2);
	this->msg << value3;

	this->checkSize(this->valueSize*3);
	ASSERT_EQ(value2, *this->getBuffer());
	ASSERT_EQ(this->value, *(this->getBuffer()+1));
	ASSERT_EQ(value3, *(this->getBuffer()+2));
}
