#include <PcmData/PcmData.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <tuple>
#include <algorithm>
#include <atlbase.h>

using namespace ::testing;

using PcmDataEnumeratorUnitTestDataType = std::tuple<PcmDataEnumerator::SampleDataTypeProperty, PcmDataEnumerator::WaveGeneratorProperty>;

// Class for PcmDataEnumerator test with all combination of SampleDataType and WaveGenerator
class PcmDataEnumeratorUnitTest : public TestWithParam<PcmDataEnumeratorUnitTestDataType>
{
public:
	struct Name {
		struct Replace {
			void operator()(char& c) const {
				if(c == ' ') c = '_';
			}
		};

		std::string operator()(const TestParamInfo<PcmDataEnumeratorUnitTestDataType>& params) {
			auto& sp(std::get<0>(params.param));
			auto& wp(std::get<1>(params.param));

			char buff[100];
			auto len = sprintf_s(buff, "%02d_%s_%s", params.index, sp.name, wp.waveForm);
			std::for_each(buff, &buff[len], Replace());
			return buff;
		}
	};
};

TEST_P(PcmDataEnumeratorUnitTest, SampleDataType)
{
	auto& param(GetParam());
	auto& sp(std::get<0>(param));
	EXPECT_NE(sp.type, IPcmData::SampleDataType::Unknown);
}

TEST_P(PcmDataEnumeratorUnitTest, WaveGenerator)
{
	auto& param(GetParam());
	auto& sp(std::get<0>(param));
	auto& wp(std::get<1>(param));

	auto gen = wp.factory(sp.type, 0);
	ASSERT_THAT(gen, NotNull());
	CComPtr<IPcmData> pcmData(createPcmData(440, 1, gen));
	ASSERT_THAT(pcmData.p, NotNull());

	EXPECT_EQ(pcmData->getSampleDataType(), sp.type);
	EXPECT_EQ(pcmData->getBitsPerSample(), sp.bitsPerSample);
	EXPECT_EQ(pcmData->getFormatTag(), sp.formatTag);
	EXPECT_STREQ(pcmData->getWaveForm(), wp.waveForm);
}

INSTANTIATE_TEST_SUITE_P(AllProperties, PcmDataEnumeratorUnitTest,
	Combine(
		ValuesIn(PcmDataEnumerator::getSampleDatatypeProperties()),
		ValuesIn(PcmDataEnumerator::getWaveGeneratorProperties())
	),
	PcmDataEnumeratorUnitTest::Name()
);
