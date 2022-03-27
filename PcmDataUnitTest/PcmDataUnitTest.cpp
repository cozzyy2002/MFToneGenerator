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

	INT32 highValue;
	INT32 zeroValue;
	INT32 lowValue;
	INT32 height;

	PcmDataUnitTest()
		: sp(std::get<0>(GetParam())), wp(std::get<1>(GetParam()))
		, samplesPerSec(std::get<2>(GetParam())), channels(std::get<3>(GetParam()))
		, key(std::get<4>(GetParam())), phaseShift(std::get<5>(GetParam()))
	{}

	void SetUp() {
		highValue = IPcmSample::getHighValue(sp.type).getInt32();
		zeroValue = IPcmSample::getZeroValue(sp.type).getInt32();
		lowValue = IPcmSample::getLowValue(sp.type).getInt32();
		height = highValue - zeroValue;
	}

	void getExpectedTotal(size_t sampleCount, INT32& unsignedTotal) {
		switch(wp.type) {
		case IPcmData::WaveFormType::SquareWave:
			unsignedTotal = height * sampleCount;
			break;
		case IPcmData::WaveFormType::SineWave:
			unsignedTotal = (INT32)(height * sampleCount * 2 / 3.141592);
			break;
		case IPcmData::WaveFormType::TriangleWave:
			unsignedTotal = height * sampleCount / 2;
			break;
		default:
			FAIL() << "Unknown wave form: " << (int)wp.type;
			break;
		}
	}

	void getTotal(IPcmSample* pcmSample, INT32& unsignedTotal, INT32& signedTotal) {
		unsignedTotal = signedTotal = 0;
		auto count = pcmSample->getSampleCount();
		for(size_t i = 0; i < count; i++) {
			auto value = (*pcmSample)[i].getInt32();
			unsignedTotal += std::abs(value) - zeroValue;
			signedTotal += value - zeroValue;
		}
	}

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
	// Create IPcmData object and generate 1-cycle PCM samples.
	auto gen = wp.factory(sp.type, 0);
	CComPtr<IPcmData> pcmData = createPcmData(samplesPerSec, channels, gen);
	ASSERT_THAT(pcmData, Not(nullptr));
	pcmData->generate(key, 1.0f, phaseShift);
	auto bufferSize = pcmData->getSampleBufferSize(0);
	auto buffer = std::make_unique<BYTE[]>(bufferSize);
	auto sampleCount = pcmData->getSamplesPerCycle();
	auto bytesPerSample = pcmData->getBitsPerSample() / 8;
	ASSERT_EQ(pcmData->getBlockAlign(), bytesPerSample * channels);
	ASSERT_EQ(bufferSize / bytesPerSample, sampleCount);

	std::unique_ptr<IPcmSample> pcmSample;
	INT32 expectedTotal, unsignedTotal, signedTotal;
	getExpectedTotal(sampleCount, expectedTotal);

	// Test value of Samples in the IPcmData.
	pcmSample.reset(createPcmSample(pcmData));
	ASSERT_THAT(pcmSample.get(), Not(nullptr));
	getTotal(pcmSample.get(), unsignedTotal, signedTotal);
	EXPECT_EQ(unsignedTotal, expectedTotal);
	EXPECT_EQ(signedTotal, 0);

	// Test value of Samples copied to the buffer.
	ASSERT_HRESULT_SUCCEEDED(pcmData->copyTo(buffer.get(), bufferSize));
	pcmSample.reset(createPcmSample(sp.type, buffer.get(), bufferSize));
	ASSERT_THAT(pcmSample.get(), Not(nullptr));
	getTotal(pcmSample.get(), unsignedTotal, signedTotal);
	EXPECT_EQ(unsignedTotal, expectedTotal);
	EXPECT_EQ(signedTotal, 0);
}

INSTANTIATE_TEST_SUITE_P(all, PcmDataUnitTest,
	Combine(
		ValuesIn(PcmDataEnumerator::getSampleDatatypeProperties()),
		ValuesIn(PcmDataEnumerator::getWaveFormProperties()),
		Values(44100/*, 32000*/),										// Samples/Second
		Values(1/*, 2, 4*/),											// Channels
		Values(440, 600),												// Key
		Values(0, 0.2/*, 0.5*/)											// Phase shift
	),
	PcmDataUnitTest::Name()
);
