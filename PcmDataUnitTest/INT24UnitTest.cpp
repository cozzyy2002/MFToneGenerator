#include <PcmData/INT24.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <tuple>

using namespace ::testing;

// Size of INT24 should be 24bit(3byte).
TEST(BasicTest, size)
{
	static const int expectedSize = 3;
	ASSERT_EQ(sizeof(INT24), expectedSize);

	INT24 x[9];
	ASSERT_EQ(sizeof(x), expectedSize * ARRAYSIZE(x));
}

// Data in INT24 array and raw byte array should be equal.
TEST(BasicTest, array)
{
	static const BYTE byteArray[] = { 3, 2, 1, 6, 5, 4, 9, 8, 7};
	static const INT24 int24Array[] = { 0x010203, 0x40506, 0x070809 };
	ASSERT_EQ(sizeof(byteArray), sizeof(int24Array));

	auto pInt24Array = (LPBYTE)int24Array;
	for(int i = 0; i < ARRAYSIZE(byteArray); i++)
	{
		EXPECT_EQ(byteArray[i], pInt24Array[i]);
	}
}

class INT24UnitTest : public Test
{
public:
	class Testee : public INT24
	{
	public:
		Testee() {}
		Testee(INT32 value) : INT24(value) {}
		Testee(float value) : INT24(value) {}

		bool checkValue(BYTE high, BYTE mid, BYTE low) {
			EXPECT_EQ(value[0], low);
			EXPECT_EQ(value[1], mid);
			EXPECT_EQ(value[2], high);
			return (value[0] == low) && (value[1] == mid) && (value[2] == high);
		}
	};
};

// Default constructor should create instance that has 0 value.
TEST_F(INT24UnitTest, DefaultConstructor)
{
	Testee testee;
	EXPECT_TRUE(testee.checkValue(0, 0, 0));
	EXPECT_EQ((float)testee, 0.0f);
}

// class for Constructor test with parameters.
// Prammeters are followings in the tupple<> object.
//   INT32 = value to be passed to the constructor.
//   1st BYTE = Highest byte of expected value.
//   2nd BYTE = Middle byte of expected value.
//   3ed BYTE = Lowest byte of expected value.
class INT24ConstructorUnitTest : public INT24UnitTest, public WithParamInterface<std::tuple<INT32, BYTE, BYTE, BYTE>> {};

// Constructor with INT32/float parameter should create instance
// that has value equal to the parameter.
TEST_P(INT24ConstructorUnitTest, WithParameter)
{
	auto& params = GetParam();
	auto value = std::get<0>(params);

	Testee fromINT32(value);
	EXPECT_TRUE(fromINT32.checkValue(std::get<1>(params), std::get<2>(params), std::get<3>(params)));
	EXPECT_EQ((float)fromINT32, (float)value);

	Testee fromFloat((float)value);
	EXPECT_TRUE(fromFloat.checkValue(std::get<1>(params), std::get<2>(params), std::get<3>(params)));
	EXPECT_EQ((float)fromFloat, (float)value);
}

INSTANTIATE_TEST_SUITE_P(_, INT24ConstructorUnitTest, Values(
	std::make_tuple(0, 0x00, 0x00, 0x00),
	std::make_tuple(-1, 0xff, 0xff, 0xff),
	std::make_tuple(INT24UnitTest::Testee::MinValue, 0x80, 0x00, 0x00),
	std::make_tuple(INT24UnitTest::Testee::MaxValue, 0x7f, 0xff, 0xff),
	std::make_tuple(0x010203, 0x01, 0x02, 0x03),
	std::make_tuple(-0x010203, 0xfe, 0xfd, 0xfd)
));

class INT24BinaryOperatorUnitTest : public INT24UnitTest, public WithParamInterface<std::tuple<INT32, INT32>> {};

TEST_P(INT24BinaryOperatorUnitTest, _)
{
	auto& params = GetParam();
	auto a = std::get<0>(params);
	auto b = std::get<1>(params);

	EXPECT_EQ(INT24(a) + INT24(b), a + b);
	EXPECT_EQ(INT24(a) + float(b), a + b);
	EXPECT_EQ(INT24(a) - INT24(b), a - b);
	EXPECT_EQ(INT24(a) - float(b), a - b);
	EXPECT_EQ(INT24(a) * float(b), a * b);
	EXPECT_EQ(double(a) * INT24(b), a * b);
}

INSTANTIATE_TEST_SUITE_P(_, INT24BinaryOperatorUnitTest, Values(
	// Note: 0x0123 * 0x4567 = 0x4ee415: does not exceed INT24::MaxValue.
	std::make_tuple(0x0123, 0x4567),
	std::make_tuple(-0x0123, 0x4567),
	std::make_tuple(0x0123, -0x4567),
	std::make_tuple(-0x0123, -0x4567),
	std::make_tuple(0, 0),
	std::make_tuple(0, 0x1234),
	std::make_tuple(0x1234, 0)
));
