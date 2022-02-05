#include <PcmData/INT24.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <tuple>

using namespace ::testing;

TEST(BasicTest, size)
{
	static const int expectedSize = 3;
	ASSERT_EQ(sizeof(INT24), expectedSize);

	INT24 x[9];
	ASSERT_EQ(sizeof(x), expectedSize * ARRAYSIZE(x));
}

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
			EXPECT_EQ((BYTE)value[0], low);
			EXPECT_EQ((BYTE)value[1], mid);
			EXPECT_EQ((BYTE)value[2], high);
			return !HasFailure();
		}
	};
};

TEST_F(INT24UnitTest, DefaultConstructor)
{
	Testee testee;
	EXPECT_TRUE(testee.checkValue(0, 0, 0));
}

// class for Constructor test with parameters.
// Prammeters are followings in the tupple<> object.
//   INT32 = value to be passed to the constructor.
//   1st BYTE = Highest byte of expected value.
//   2nd BYTE = Middle byte of expected value.
//   3ed BYTE = Lowest byte of expected value.
class INT24ConstructorUnitTest : public INT24UnitTest, public WithParamInterface<std::tuple<INT32, BYTE, BYTE, BYTE>> {};

TEST_P(INT24ConstructorUnitTest, WithParameter)
{
	auto& params = GetParam();

	Testee fromINT32(std::get<0>(params));
	EXPECT_TRUE(fromINT32.checkValue(std::get<1>(params), std::get<2>(params), std::get<3>(params)));

	Testee fromFloat((float)std::get<0>(params));
	EXPECT_TRUE(fromFloat.checkValue(std::get<1>(params), std::get<2>(params), std::get<3>(params)));
}

INSTANTIATE_TEST_SUITE_P(_, INT24ConstructorUnitTest, Values(
	std::make_tuple(0, 0x00, 0x00, 0x00),
	std::make_tuple(-1, 0xff, 0xff, 0xff),
	std::make_tuple(INT24UnitTest::Testee::MinValue, 0x80, 0x00, 0x00),
	std::make_tuple(INT24UnitTest::Testee::MaxValue, 0x7f, 0xff, 0xff),
	std::make_tuple(0x010203, 0x01, 0x02, 0x03),
	std::make_tuple(-0x010203, 0xfe, 0xfd, 0xfd)
));
