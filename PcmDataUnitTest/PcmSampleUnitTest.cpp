#include "PcmData/PcmSample.h"
#include "PcmData/INT24.h"
#include "PcmData/PcmDataImpl.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <mmreg.h>
#include <memory>

using namespace ::testing;

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

template<> const float TestSample<float>::Samples[] = {  -1.0f, -0.8f, 0 , 0.8f, 1.0f };
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
		// Copy TestSample to buffer of type T.
		testSamples = TestSample<T>::Samples;
		testSamplesCount = TestSample<T>::SamplesCount;
		buffer = std::make_unique<T[]>(testSamplesCount);
		for(size_t i = 0; i < testSamplesCount; i++) { buffer[i] = (T)testSamples[i]; }

		// Create IPcmSample object using the buffer.
		testee.reset(createPcmSample<T>(buffer.get(), testSamplesCount * sizeof(T)));
		ASSERT_THAT(testee.get(), Not(nullptr));
	}
};

using AllSampleDataTypes = Types<UINT8, INT16, INT24, float>;
TYPED_TEST_SUITE(PcmSampleTypedTest, AllSampleDataTypes);

// Test for IPcmSample::isValue(index).
// The method should return:
//   true if index = 0 ~ (sample count - 1)
//   false otherwise
TYPED_TEST(PcmSampleTypedTest, isValid)
{
	EXPECT_TRUE(this->testee->isValid(0));
	EXPECT_TRUE(this->testee->isValid(this->testSamplesCount - 1));
	EXPECT_FALSE(this->testee->isValid(this->testSamplesCount));
}

// Test for INT32/double operator of IPcmSample::Value class.
// The operator should return value as same as sample data passed to createPcmSample() functio.
TYPED_TEST(PcmSampleTypedTest, to_int_float)
{
	for(size_t i = 0; i < this->testSamplesCount; i++) {
		auto value = (*this->testee)[i];
		auto formatTag = this->testee->getFormatTag();
		switch(formatTag) {
		case WAVE_FORMAT_PCM:
			EXPECT_EQ((INT32)value, this->testSamples[i]);
			break;
		case WAVE_FORMAT_IEEE_FLOAT:
			EXPECT_EQ((double)value, this->testSamples[i]);
			break;
		default:
			FAIL() << "Unknown FormatTag: " << formatTag;
			break;
		}
	}
}

// Test for std::string operator of IPcmSample::Value class.
// The operator should return string that is parsed to the value as same as sample data passed to createPcmSample() functio.
TYPED_TEST(PcmSampleTypedTest, to_string)
{
	for(size_t i = 0; i < this->testSamplesCount; i++) {
		auto value = (*this->testee)[i];
		auto str = (std::string)value;
		auto formatTag = this->testee->getFormatTag();
		switch(formatTag) {
		case WAVE_FORMAT_PCM:
			EXPECT_EQ((TypeParam)atoi(str.c_str()), this->testSamples[i]);
			break;
		case WAVE_FORMAT_IEEE_FLOAT:
			EXPECT_EQ((TypeParam)atof(str.c_str()), this->testSamples[i]);
			break;
		default:
			FAIL() << "Unknown FormatTag: " << formatTag;
			break;
		}
	}
}

// Test for methods of IPcmSample class that returns constant value.
// The methods should return value as same as value of PcmData<T> class.
TYPED_TEST(PcmSampleTypedTest, constants)
{
	auto formatTag = this->testee->getFormatTag();
	switch(formatTag) {
	case WAVE_FORMAT_PCM:
		EXPECT_EQ((INT32)this->testee->getHighValue(), PcmData<TypeParam>::HighValue);
		EXPECT_EQ((INT32)this->testee->getZeroValue(), PcmData<TypeParam>::ZeroValue);
		EXPECT_EQ((INT32)this->testee->getLowValue(), PcmData<TypeParam>::LowValue);
		break;
	case WAVE_FORMAT_IEEE_FLOAT:
		EXPECT_EQ((double)this->testee->getHighValue(), PcmData<TypeParam>::HighValue);
		EXPECT_EQ((double)this->testee->getZeroValue(), PcmData<TypeParam>::ZeroValue);
		EXPECT_EQ((double)this->testee->getLowValue(), PcmData<TypeParam>::LowValue);
		break;
	default:
		FAIL() << "Unknown FormatTag: " << formatTag;
		break;
	}
}

// createPcmSample() with Null IPcmData parameter should return null.
TEST(PcmSampleUnitTest, IPcmData_null)
{
	std::unique_ptr<IPcmSample> testee(createPcmSample(nullptr));
	ASSERT_EQ(testee.get(), nullptr);
}

// createPcmSample() with Null IPcmData that has not been called generate() method, should return null.
TEST(PcmSampleUnitTest, IPcmData_not_generated)
{
	auto gen = createSineWaveGenerator(IPcmData::SampleDataType::PCM_16bits);
	CComPtr<IPcmData> pcmData(createPcmData(44100, 1, gen));
	std::unique_ptr<IPcmSample> testee(createPcmSample(pcmData));
	ASSERT_EQ(testee.get(), nullptr);
}

// createPcmSample() with Null unknown sample data type parameter should return null.
TEST(PcmSampleUnitTest, error_unknown_data_type)
{
	BYTE buffer[10];
	std::unique_ptr<IPcmSample> testee(createPcmSample(IPcmData::SampleDataType::Unknown, buffer, sizeof(buffer)));
	ASSERT_EQ(testee.get(), nullptr);
}

// createPcmSample() with Null buffer parameter should return null.
TEST(PcmSampleUnitTest, error_buffer_null)
{
	std::unique_ptr<IPcmSample> testee(createPcmSample<INT16>(nullptr, 10));
	ASSERT_EQ(testee.get(), nullptr);
}

// createPcmSample() with illegal boundary buffer size parameter should return null.
TYPED_TEST(PcmSampleTypedTest, error_buffer_size)
{
	if(sizeof(TypeParam) == 1) return;

	// Illegal buffer size for the samples other than UINT8
	BYTE buffer[11];
	std::unique_ptr<IPcmSample> testee(createPcmSample<TypeParam>(buffer, sizeof(buffer)));
	ASSERT_EQ(testee.get(), nullptr);
}
