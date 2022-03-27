#include <PcmData/PcmData.h>
#include <PcmData/PcmSample.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <tuple>
#include <atlbase.h>

using namespace ::testing;

using PcmDataUnitTestDataType = std::tuple<
	PcmDataEnumerator::SampleDataTypeProperty,
	PcmDataEnumerator::WaveFormProperty,
	DWORD,											// Samples/Second
	WORD,											// Channels
	float,											// Key
	float											// Phase shift
>;

class PcmDataUnitTest : public TestWithParam<PcmDataUnitTestDataType>
{
public:
	const PcmDataEnumerator::SampleDataTypeProperty& sp;
	const PcmDataEnumerator::WaveFormProperty& wp;
	const DWORD samplesPerSec;
	const WORD channels;
	const float key;
	const float phaseShift;

	static INT32 absoluteTotal;
	static INT32 total;

	static void SetUpTestCase() {
	}

	PcmDataUnitTest()
		: sp(std::get<0>(GetParam())), wp(std::get<1>(GetParam()))
		, samplesPerSec(std::get<2>(GetParam())), channels(std::get<3>(GetParam()))
		, key(std::get<4>(GetParam())), phaseShift(std::get<5>(GetParam()))
	{}

	void SetUp() {}

	// Creates test name that contains SampleDataType and WaveForm.
	// Space characters are replaced by "_".
	struct Name {
		std::string operator()(const TestParamInfo<PcmDataUnitTestDataType>& params) {
			auto& sp(std::get<0>(params.param));
			auto& wp(std::get<1>(params.param));
			auto samplesPerSec(std::get<2>(params.param));
			auto channels(std::get<3>(params.param));
			auto key(std::get<4>(params.param));
			auto phaseShift(std::get<5>(params.param));

			char buff[100];
			auto len = sprintf_s(buff, "%02d_%s_%s_%dHz_%dch_%d_%d",
				params.index, wp.name, sp.name, samplesPerSec, channels, (int)key, (int)(phaseShift * 100));
			std::replace(buff, &buff[len], ' ', '_');
			return buff;
		}
	};
};

TEST_P(PcmDataUnitTest, total)
{
}

INSTANTIATE_TEST_SUITE_P(all, PcmDataUnitTest,
	Combine(
		ValuesIn(PcmDataEnumerator::getSampleDatatypeProperties()),
		ValuesIn(PcmDataEnumerator::getWaveFormProperties()),
		Values(44100/*, 32000*/),
		Values(1/*, 2, 4*/),
		Values(440, 600),
		Values(0, 0.2/*, 0.5*/)
	),
	PcmDataUnitTest::Name()
);
