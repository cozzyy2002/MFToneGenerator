#include <PcmData/PcmData.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <tuple>
#include <atlbase.h>

using namespace ::testing;

TEST(PcmDataEnumeratorUnitTest, SampleDataTypeProperties)
{
	auto properties = PcmDataEnumerator::getSampleDatatypeProperties();
	ASSERT_EQ(properties.size(), 4);

	for(auto sp : properties) {
		EXPECT_THAT(sp.type, AnyOf(
			IPcmData::SampleDataType::PCM_8bits,
			IPcmData::SampleDataType::PCM_16bits,
			IPcmData::SampleDataType::PCM_24bits,
			IPcmData::SampleDataType::IEEE_Float
		));
	}
}

TEST(PcmDataEnumeratorUnitTest, WaveFormProperties)
{
	auto properties = PcmDataEnumerator::getWaveFormProperties();
	ASSERT_EQ(properties.size(), 3);

	for(auto wp : properties) {
		EXPECT_THAT(wp.type, AnyOf(
			IPcmData::WaveFormType::SquareWave,
			IPcmData::WaveFormType::SineWave,
			IPcmData::WaveFormType::TriangleWave
		));
	}
}

using PcmDataEnumeratorUnitTestDataType = std::tuple<PcmDataEnumerator::SampleDataTypeProperty, PcmDataEnumerator::WaveFormProperty>;

// Class for PcmDataEnumerator test with all combination of SampleDataType and WaveForm
class PcmDataEnumeratorUnitTest : public TestWithParam<PcmDataEnumeratorUnitTestDataType>
{
public:
	// Creates test name that contains SampleDataType and WaveForm.
	// Space characters are replaced by "_".
	struct Name {
		std::string operator()(const TestParamInfo<PcmDataEnumeratorUnitTestDataType>& params) {
			auto& sp(std::get<0>(params.param));
			auto& wp(std::get<1>(params.param));

			char buff[100];
			auto len = sprintf_s(buff, "%02d_%s_%s", params.index, sp.name, wp.name);
			std::replace(buff, &buff[len], ' ', '_');
			return buff;
		}
	};
};

// WaveGenerator and PcmData object should be created for all combination of
// SampleDataType and WaveFormType.
// And created objects should have property value that is equal to creation parameter.
TEST_P(PcmDataEnumeratorUnitTest, WaveGenerator)
{
	auto& param(GetParam());
	auto& sp(std::get<0>(param));	// SampleDataTypeProperty
	auto& wp(std::get<1>(param));	// WaveFormProperty

	static const DWORD samplesPerSec = 44100;
	static const WORD channels = 2;
	auto gen = wp.factory(sp.type, 0);
	ASSERT_THAT(gen, NotNull());
	CComPtr<IPcmData> pcmData(createPcmData(samplesPerSec, channels, gen));
	ASSERT_THAT(pcmData.p, NotNull());

	EXPECT_EQ(pcmData->getSamplesPerSec(), samplesPerSec);
	EXPECT_EQ(pcmData->getChannels(), channels);
	EXPECT_EQ(pcmData->getSampleDataType(), sp.type);
	EXPECT_STREQ(pcmData->getSampleDataTypeName(), sp.name);
	EXPECT_EQ(pcmData->getBitsPerSample(), sp.bitsPerSample);
	EXPECT_EQ(pcmData->getFormatTag(), sp.formatTag);
	EXPECT_EQ(pcmData->getWaveFormType(), wp.type);
	EXPECT_STREQ(pcmData->getWaveFormTypeName(), wp.name);
}

INSTANTIATE_TEST_SUITE_P(AllProperties, PcmDataEnumeratorUnitTest,
	Combine(
		ValuesIn(PcmDataEnumerator::getSampleDatatypeProperties()),
		ValuesIn(PcmDataEnumerator::getWaveFormProperties())
	),
	PcmDataEnumeratorUnitTest::Name()
);
