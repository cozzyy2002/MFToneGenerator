#include "PcmData/PcmSample.h"
#include "PcmData/INT24.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <mmreg.h>
#include <memory>

using namespace ::testing;

using AllSampleDataTypes = Types<UINT8, INT16, INT24, float>;

template<typename T>
struct TestSample
{
	static const T Samples[];
	static const size_t SamplesCount;
};

template<> const UINT8 TestSample<UINT8>::Samples[] = { 0, 0x40, 0x80, 0xc0, 0xff };
template<> const size_t TestSample<UINT8>::SamplesCount = ARRAYSIZE(Samples);

template<> const INT16 TestSample<INT16>::Samples[] = { (INT16)0xa000, 0 , 0x6000, (INT16)0xffff };
template<> const size_t TestSample<INT16>::SamplesCount = ARRAYSIZE(Samples);

template<> const INT24 TestSample<INT24>::Samples[] = { 0xa00000, 0 , 0x600000, 0xffffff };
template<> const size_t TestSample<INT24>::SamplesCount = ARRAYSIZE(Samples);

template<> const float TestSample<float>::Samples[] = {  -0.8f, 0 , 0.8f };
template<> const size_t TestSample<float>::SamplesCount = ARRAYSIZE(Samples);

template<typename T>
class PcmSampleTypedTest : public Test
{
public:
	std::unique_ptr<IPcmSample> testee;
	std::unique_ptr<T[]> buffer;
	const T* testSamples;
	size_t testSamplesCount;

	void SetUp() {
		testSamples = TestSample<T>::Samples;
		testSamplesCount = TestSample<T>::SamplesCount;
		buffer = std::make_unique<T[]>(testSamplesCount);
		for(size_t i = 0; i < testSamplesCount; i++) { buffer[i] = (T)testSamples[i]; }
		testee.reset(createPcmSample<T>(buffer.get(), testSamplesCount * sizeof(T)));
		ASSERT_THAT(testee.get(), Not(nullptr));
	}
};

TYPED_TEST_SUITE(PcmSampleTypedTest, AllSampleDataTypes);

TYPED_TEST(PcmSampleTypedTest, isValid)
{
	EXPECT_TRUE(this->testee->isValid(0));
	EXPECT_TRUE(this->testee->isValid(this->testSamplesCount - 1));
	EXPECT_FALSE(this->testee->isValid(this->testSamplesCount));
}

TYPED_TEST(PcmSampleTypedTest, getValue)
{
	for(size_t i = 0; i < this->testSamplesCount; i++) {
		auto value = (*this->testee)[i];
		switch(this->testee->getFormatTag()) {
		case WAVE_FORMAT_PCM:
			EXPECT_EQ((INT32)value, this->testSamples[i]);
			break;
		case WAVE_FORMAT_IEEE_FLOAT:
			EXPECT_EQ((double)value, this->testSamples[i]);
			break;
		default:
			FAIL() << "Unknown FormatTag: " << this->testee->getFormatTag();
			break;
		}
	}
}
