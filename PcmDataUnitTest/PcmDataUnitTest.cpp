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
	INT32 positiveHeight;
	INT32 negativeHeight;

	PcmDataUnitTest()
		: sp(std::get<0>(GetParam())), wp(std::get<1>(GetParam()))
		, samplesPerSec(std::get<2>(GetParam())), channels(std::get<3>(GetParam()))
		, key(std::get<4>(GetParam())), phaseShift(std::get<5>(GetParam()))
	{
		highValue = IPcmSample::getHighValue(sp.type).getInt32();
		zeroValue = IPcmSample::getZeroValue(sp.type).getInt32();
		lowValue = IPcmSample::getLowValue(sp.type).getInt32();
		positiveHeight = highValue - zeroValue;
		negativeHeight = zeroValue - lowValue;
	}

	void getExpectedTotal(size_t sampleCount, double& unsignedTotal) {
		double squreTotal = (double)(positiveHeight + negativeHeight) * sampleCount / channels / 2;
		switch(wp.type) {
		case IPcmData::WaveFormType::SquareWave:
			unsignedTotal = squreTotal;
			break;
		case IPcmData::WaveFormType::SineWave:
			unsignedTotal = (double)squreTotal * 2 / 3.141592;
			break;
		case IPcmData::WaveFormType::TriangleWave:
			unsignedTotal = squreTotal / 2;
			break;
		default:
			FAIL() << "Unknown wave form: " << (int)wp.type;
			break;
		}
	}

	void getTotal(IPcmSample* pcmSample, WORD channel, double& unsignedTotal, double& signedTotal) {
		unsignedTotal = signedTotal = 0;
		for(size_t i = 0; i < pcmSample->getSampleCount(); i += channels) {
			ASSERT_TRUE(pcmSample->isValid(i + channel)) << "isValid(" << i << " + " << channel << ")";
			auto value = (*pcmSample)[i + channel].getInt32() - zeroValue;
			unsignedTotal += std::abs(value);
			signedTotal += value;
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
			auto len = sprintf_s(buff, "%d_%s_%s_%dHz_%dch_%d_%d",
				params.index, wp.name, sp.name, samplesPerSec, channels, (int)key, (int)(phaseShift * 100));
			std::replace(buff, &buff[len], ' ', '_');
			return buff;
		}
	};
};

TEST_P(PcmDataUnitTest, total)
{
	// Create IPcmData object and generate 1-cycle PCM samples.
	auto gen = wp.factory(sp.type, wp.defaultPrameter);
	auto pcmData = createPcmData(samplesPerSec, channels, gen);
	ASSERT_THAT(pcmData, NotNull());
	pcmData->generate(key, 1.0f, phaseShift);
	auto sampleCount = pcmData->getSamplesPerCycle();
	auto bytesPerSample = pcmData->getBitsPerSample() / 8;
	ASSERT_EQ(pcmData->getBlockAlign(), bytesPerSample * channels);

	std::unique_ptr<IPcmSample> pcmSample;
	double expectedTotal, unsignedTotal, signedTotal;
	getExpectedTotal(sampleCount, expectedTotal);

	// Test value of Samples in the IPcmData.
	pcmSample.reset(createPcmSample(pcmData));
	ASSERT_THAT(pcmSample, NotNull());
	ASSERT_EQ(pcmSample->getSampleCount(), sampleCount);
	for(WORD ch = 0; ch < channels; ch++) {
		getTotal(pcmSample.get(), ch, unsignedTotal, signedTotal);
		EXPECT_NEAR(unsignedTotal, expectedTotal, positiveHeight + negativeHeight)	<< "PcmData: Unsigned total: channel=" << (ch + 1);
		EXPECT_NEAR(signedTotal, 0, positiveHeight)									<< "PcmData: Signed total: chennel=" << (ch + 1);
	}

	// Copy samples generated to the buffer.
	auto bufferSize = pcmData->getSampleBufferSize(0);
	ASSERT_EQ(bufferSize / bytesPerSample, sampleCount);
	auto buffer = std::make_unique<BYTE[]>(bufferSize);
	ASSERT_HRESULT_SUCCEEDED(pcmData->copyTo(buffer.get(), bufferSize));

	// Test value of Samples copied to the buffer.
	pcmSample.reset(createPcmSample(sp.type, buffer.get(), bufferSize));
	ASSERT_THAT(pcmSample, NotNull());
	ASSERT_EQ(pcmSample->getSampleCount(), sampleCount);
	for(WORD ch = 0; ch < channels; ch++) {
		getTotal(pcmSample.get(), ch, unsignedTotal, signedTotal);
		EXPECT_NEAR(unsignedTotal, expectedTotal, positiveHeight + negativeHeight)	<< "Copied buffer: Unsigned total: channel=" << (ch + 1);
		EXPECT_NEAR(signedTotal, 0, positiveHeight)									<< "Copied buffer: Signed total: channel=" << (ch + 1);
	}
}

INSTANTIATE_TEST_SUITE_P(all, PcmDataUnitTest,
	Combine(
		ValuesIn(PcmDataEnumerator::getSampleDatatypeProperties()),
		ValuesIn(PcmDataEnumerator::getWaveFormProperties()),
		Values(44100, 32000),										// Samples/Second
		Values(1, 2, 4),											// Channels
		Values(440, 600),											// Key
		Values(0, 0.2, 0.5)											// Phase shift
	),
	PcmDataUnitTest::Name()
);


using PcmDataBufferUnitTestDataType = std::tuple<
	PcmDataEnumerator::SampleDataTypeProperty,
	DWORD,											// Samples/Second
	WORD,											// Channels
	float,											// Key
	size_t											// duration
>;

class PcmDataBufferUnitTest : public TestWithParam<PcmDataBufferUnitTestDataType>
{
public:
	const PcmDataEnumerator::SampleDataTypeProperty& sp;
	const DWORD samplesPerSec;
	const WORD channels;
	const float key;
	const size_t duration;

	PcmDataBufferUnitTest()
		: sp(std::get<0>(GetParam()))
		, samplesPerSec(std::get<1>(GetParam())), channels(std::get<2>(GetParam()))
		, key(std::get<3>(GetParam())), duration(std::get<4>(GetParam()))
	{}

	struct Name {
		std::string operator()(const TestParamInfo<PcmDataBufferUnitTestDataType>& params) {
			auto& sp(std::get<0>(params.param));
			auto samplesPerSec(std::get<1>(params.param));
			auto channels(std::get<2>(params.param));
			auto key(std::get<3>(params.param));
			auto duration(std::get<4>(params.param));

			char buff[100];
			auto len = sprintf_s(buff, "%d_%s_%dHz_%dch_%d_%d",
				params.index, sp.name, samplesPerSec, channels, (int)key, (int)duration);
			std::replace(buff, &buff[len], ' ', '_');
			return buff;
		}
	};
};

TEST_P(PcmDataBufferUnitTest, buffer)
{
	// Create IPcmData object and generate 1-cycle PCM samples.
	auto gen = createTriangleWaveGenerator(sp.type);
	auto pcmData = createPcmData(samplesPerSec, channels, gen);
	ASSERT_THAT(pcmData, NotNull());
	pcmData->generate(key, 1.0f, 0.0f);
	auto bytesPerSample = pcmData->getBitsPerSample() / 8;
	ASSERT_EQ(pcmData->getBlockAlign(), bytesPerSample * channels);

	// IPcmSample created from 1-cycle data in IPcmData.
	std::unique_ptr<IPcmSample> pcmSampleSrc(createPcmSample(pcmData));
	ASSERT_THAT(pcmSampleSrc, NotNull());
	ASSERT_EQ(pcmSampleSrc->getSampleCount(), pcmData->getSamplesPerCycle());

	// Allocate buffer enough for duration, and copy generated samples to the buffer.
	auto bufferSize = pcmData->getSampleBufferSize(duration);
	ASSERT_EQ(bufferSize % pcmData->getBlockAlign(), 0);
	auto buffer = std::make_unique<BYTE[]>(bufferSize);
	ASSERT_HRESULT_SUCCEEDED(pcmData->copyTo(buffer.get(), bufferSize));

	// IPcmSample created from the buffer.
	std::unique_ptr<IPcmSample> pcmSampleDest(createPcmSample(sp.type, buffer.get(), bufferSize));
	ASSERT_THAT(pcmSampleDest, NotNull());
	auto samplesInBuffer = pcmSampleDest->getSampleCount();
	ASSERT_EQ(samplesInBuffer, bufferSize / bytesPerSample);

	// Compare samples in IPcmData object and in the buffer.
	auto samplesPerCycle = pcmData->getSamplesPerCycle();
	for(size_t i = 0; i < samplesInBuffer; i++) {
		auto ii = i % samplesPerCycle;
		ASSERT_TRUE(pcmSampleSrc->isValid(ii)) << "pcmSampleSrc->isValid(" << ii << ")";
		ASSERT_TRUE(pcmSampleDest->isValid(i)) << "pcmSampleDest->isValid(" << i << ")";
		auto src = (*pcmSampleSrc)[ii];
		auto dest = (*pcmSampleDest)[i];
		ASSERT_EQ(src.getInt32(), dest.getInt32()) << "Source sample[" << ii << "], Dest sample[" << i << "]";
	}
}

INSTANTIATE_TEST_SUITE_P(all, PcmDataBufferUnitTest,
	Combine(
		ValuesIn(PcmDataEnumerator::getSampleDatatypeProperties()),
		Values(44100, 32000),											// Samples/Second
		Values(1, 2, 4),												// Channels
		Values(440, 600),												// Key
		Values(1000, 100, 20)											// duration
	),
	PcmDataBufferUnitTest::Name()
);
