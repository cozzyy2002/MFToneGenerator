#include "PcmData/PcmSample.h"
#include "PcmData/INT24.h"
#include "PcmData/PcmDataImpl.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <mmreg.h>
#include <memory>

using namespace ::testing;

template<typename T>
IPcmData::SampleDataType getSampleDataType()
{
	switch(sizeof(T)) {
	case 1:
		return IPcmData::SampleDataType::PCM_8bits;
	case 2:
		return IPcmData::SampleDataType::PCM_16bits;
	case 3:
		return IPcmData::SampleDataType::PCM_24bits;
	case 4:
		return IPcmData::SampleDataType::IEEE_Float;
	default:
		return IPcmData::SampleDataType::Unknown;
	}
}

template<typename T>
struct TestSample
{
	static T Samples[];
};

template<> UINT8 TestSample<UINT8>::Samples[] = { 0, 0x40, 0x60, 0x80, 0xa0, 0xc0, 0xff };
template<> INT16 TestSample<INT16>::Samples[] = { (INT16)0xa000, (INT16)0xc000, 0, 0x6000, 0x7fff, (INT16)0xffff };
template<> INT24 TestSample<INT24>::Samples[] = { 0xa00000, 0xc00000, 0, 0x600000, 0x7fffff, 0xffffff };
template<> float TestSample<float>::Samples[] = { -1.0f, -0.4f, -0.8f, 0, 0.4f, 0.8f, 1.0f };

template<typename T>
class PcmSampleTypedTest : public Test
{
public:
	std::unique_ptr<IPcmSample> testee;
	T* testSamples;
	size_t testSamplesCount;

	void SetUp() {
		testSamples = TestSample<T>::Samples;
		testSamplesCount = ARRAYSIZE(TestSample<T>::Samples);

		// Create IPcmSample object using the buffer.
		testee.reset(createPcmSample(TestSample<T>::Samples));
		ASSERT_THAT(testee, NotNull());
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

TYPED_TEST(PcmSampleTypedTest, buffer_size)
{
	TypeParam buffer[3];
	this->testee.reset(createPcmSample(buffer));
	ASSERT_THAT(this->testee, NotNull());
	ASSERT_EQ(this->testee->getSampleCount(), ARRAYSIZE(buffer));
}

// Test for INT32/double operator of IPcmSample::Value class.
// The operator should return value as same as sample data passed to createPcmSample() function.
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
// The operator should return string that is parsed to the value as same as sample data passed to createPcmSample() function.
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
	auto sampleDataType = getSampleDataType<TypeParam>();

	switch(formatTag) {
	case WAVE_FORMAT_PCM:
		EXPECT_EQ((INT32)IPcmSample::HighValue<TypeParam>, PcmData<TypeParam>::HighValue);
		EXPECT_EQ((INT32)IPcmSample::ZeroValue<TypeParam>, PcmData<TypeParam>::ZeroValue);
		EXPECT_EQ((INT32)IPcmSample::LowValue<TypeParam>, PcmData<TypeParam>::LowValue);
		EXPECT_EQ((INT32)IPcmSample::getHighValue(sampleDataType), PcmData<TypeParam>::HighValue);
		EXPECT_EQ((INT32)IPcmSample::getZeroValue(sampleDataType), PcmData<TypeParam>::ZeroValue);
		EXPECT_EQ((INT32)IPcmSample::getLowValue(sampleDataType), PcmData<TypeParam>::LowValue);
		break;
	case WAVE_FORMAT_IEEE_FLOAT:
		EXPECT_EQ((double)IPcmSample::HighValue<TypeParam>, PcmData<TypeParam>::HighValue);
		EXPECT_EQ((double)IPcmSample::ZeroValue<TypeParam>, PcmData<TypeParam>::ZeroValue);
		EXPECT_EQ((double)IPcmSample::LowValue<TypeParam>, PcmData<TypeParam>::LowValue);
		EXPECT_EQ((double)IPcmSample::getHighValue(sampleDataType), PcmData<TypeParam>::HighValue);
		EXPECT_EQ((double)IPcmSample::getZeroValue(sampleDataType), PcmData<TypeParam>::ZeroValue);
		EXPECT_EQ((double)IPcmSample::getLowValue(sampleDataType), PcmData<TypeParam>::LowValue);
		break;
	default:
		FAIL() << "Unknown FormatTag: " << formatTag;
		break;
	}
}

TYPED_TEST(PcmSampleTypedTest, assignment)
{
	TypeParam dest = (TypeParam)0xab;
	std::unique_ptr<IPcmSample> destPcmSample(createPcmSample(&dest, 1));
	ASSERT_THAT(destPcmSample, NotNull());
	auto destValue = (*destPcmSample)[0];
	for(size_t i = 0; i < this->testSamplesCount; i++) {
		auto& testSample = this->testSamples[i];

		// Value::operator=(INT32 or double)
		destValue = this->testSamples[i];
		EXPECT_EQ(dest, testSample);
		dest = (TypeParam)0xcd;

		// Value::operator=(const Value&)
		destValue = (*this->testee)[i];
		EXPECT_EQ(dest, testSample);
		dest = (TypeParam)0xef;
	}
}

TYPED_TEST(PcmSampleTypedTest, assignment_type_error)
{
	const TypeParam expected = 10;
	TypeParam dest = expected;
	std::unique_ptr<IPcmSample> destSamples(createPcmSample(&dest, 1));
	auto destValue = (*destSamples)[0];

	std::unique_ptr<IPcmSample> sourceSamples;

	switch(sizeof(TypeParam)) {
	case 1:	// INT16 can not assign to UINT8
		{
			static INT16 source = 0;
			sourceSamples.reset(createPcmSample(&source, 1));
		}
		break;
	case 2:	// INT24 can not assign to INT16
		{
			static INT24 source = 0;
			sourceSamples.reset(createPcmSample(&source, 1));
		}
		break;
	case 3:	// float can not assign to INT24
		{
			static float source = 0;
			sourceSamples.reset(createPcmSample(&source, 1));
		}
		break;
	case 4:	// UINT8 can not assign to float
		{
			static UINT8 source = 0;
			sourceSamples.reset(createPcmSample(&source, 1));
		}
		break;
	}

	ASSERT_THAT(sourceSamples, NotNull());

	auto sourceValue = (*sourceSamples)[0];
	destValue = sourceValue;
	ASSERT_EQ(dest, expected);
}

// createPcmSample() with Null IPcmData parameter should return null.
TEST(PcmSampleUnitTest, IPcmData_null)
{
	std::shared_ptr<IPcmData> nullPcmData;
	std::unique_ptr<IPcmSample> testee(createPcmSample(nullPcmData));
	ASSERT_THAT(testee, IsNull());
}

// createPcmSample() with Null IPcmData that has not been called generate() method, should return null.
TEST(PcmSampleUnitTest, IPcmData_not_generated)
{
	auto gen = createSineWaveGenerator(IPcmData::SampleDataType::PCM_16bits);
	ASSERT_THAT(gen, NotNull());
	auto pcmData(createPcmData(44100, 1, gen));
	std::unique_ptr<IPcmSample> testee(createPcmSample(pcmData));
	ASSERT_THAT(testee, IsNull());
}

// createPcmSample() with unknown sample data type parameter should return null.
TEST(PcmSampleUnitTest, error_unknown_data_type)
{
	BYTE buffer[12];
	std::unique_ptr<IPcmSample> testee(createPcmSample(IPcmData::SampleDataType::Unknown, buffer, sizeof(buffer)));
}

// createPcmSample() with Null buffer parameter should return null.
TEST(PcmSampleUnitTest, error_buffer_null)
{
	// Buffer size that fits all sample data size.
	static const size_t bufferSize = 12;
	std::unique_ptr<IPcmSample> testee;

	testee.reset(createPcmSample<INT16>(nullptr, bufferSize));
	ASSERT_THAT(testee, IsNull());

	testee.reset(createPcmSample((INT24*)nullptr, bufferSize));
	ASSERT_THAT(testee, IsNull());

	testee.reset(createPcmSample(IPcmData::SampleDataType::IEEE_Float, nullptr, bufferSize));
	ASSERT_THAT(testee, IsNull());
}

// createPcmSample() with illegal boundary buffer size parameter should return null.
TYPED_TEST(PcmSampleTypedTest, error_buffer_size)
{
	// Illegal buffer size for the samples other than UINT8
	BYTE buffer[11];
	std::unique_ptr<IPcmSample> testee;

	auto sampleDataType = getSampleDataType<TypeParam>();

	// Boundary error does not occur when PCM_8bits.
	if(sampleDataType == IPcmData::SampleDataType::PCM_8bits) return;

	testee.reset(createPcmSample(sampleDataType, buffer, sizeof(buffer)));
	ASSERT_THAT(testee, IsNull());

	testee.reset(createPcmSample<TypeParam>(buffer, sizeof(buffer)));
	ASSERT_THAT(testee, IsNull());
}
