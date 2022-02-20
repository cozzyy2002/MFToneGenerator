#include "PcmDataImpl.h"
#include "INT24.h"

#pragma region Declaration for available T types.

template<> const IPcmData::SampleDataType WaveGenerator<UINT8>::SampleDataType = IPcmData::SampleDataType::PCM_8bits;
template<> const IPcmData::SampleDataType WaveGenerator<INT16>::SampleDataType = IPcmData::SampleDataType::PCM_16bits;
template<> const IPcmData::SampleDataType WaveGenerator<INT24>::SampleDataType = IPcmData::SampleDataType::PCM_24bits;
template<> const IPcmData::SampleDataType WaveGenerator<float>::SampleDataType = IPcmData::SampleDataType::IEEE_Float;

template<> const char* WaveGenerator<UINT8>::SampleDataTypeName = "PCM 8bit";
template<> const char* WaveGenerator<INT16>::SampleDataTypeName = "PCM 16bit";
template<> const char* WaveGenerator<INT24>::SampleDataTypeName = "PCM 24bit";
template<> const char* WaveGenerator<float>::SampleDataTypeName = "IEEE float 32bit";

const char* IWaveGenerator::SquareWaveForm = "Square Wave";
const char* IWaveGenerator::SineWaveForm = "Sine Wave";
const char* IWaveGenerator::TriangleWaveForm = "Triangle Wave";

template<> const WORD PcmData<UINT8>::FormatTag = WAVE_FORMAT_PCM;
template<> const UINT8 PcmData<UINT8>::HighValue = 0xc0;
template<> const UINT8 PcmData<UINT8>::ZeroValue = 0x80;
template<> const UINT8 PcmData<UINT8>::LowValue = 0x40;

template<> const WORD PcmData<INT16>::FormatTag = WAVE_FORMAT_PCM;
template<> const INT16 PcmData<INT16>::HighValue = 8000;
template<> const INT16 PcmData<INT16>::ZeroValue = 0;
template<> const INT16 PcmData<INT16>::LowValue = -8000;

template<> const WORD PcmData<INT24>::FormatTag = WAVE_FORMAT_PCM;
template<> const INT24 PcmData<INT24>::HighValue = 0x80000;
template<> const INT24 PcmData<INT24>::ZeroValue = 0;
template<> const INT24 PcmData<INT24>::LowValue = -0x80000;

template<> const WORD PcmData<float>::FormatTag = WAVE_FORMAT_IEEE_FLOAT;
template<> const float PcmData<float>::HighValue = 0.5f;
template<> const float PcmData<float>::ZeroValue = 0.0f;
template<> const float PcmData<float>::LowValue = -0.5f;

#pragma endregion

#pragma region Implementation of PcmDataEnumerator
template<class T>
class WaveGeneratorGetter : public WaveGenerator<T>
{
public:
	using WaveGenerator<T>::SampleDataType;
	using WaveGenerator<T>::SampleDataTypeName;
};

static const PcmDataEnumerator::SampleDataTypeProperty sampleDataTypeProperties[] = {
	{ WaveGeneratorGetter<UINT8>::SampleDataType, WaveGeneratorGetter<UINT8>::SampleDataTypeName, PcmData<UINT8>::FormatTag, sizeof(UINT8) * 8 },
	{ WaveGeneratorGetter<INT16>::SampleDataType, WaveGeneratorGetter<INT16>::SampleDataTypeName, PcmData<INT16>::FormatTag, sizeof(INT16) * 8 },
	{ WaveGeneratorGetter<INT24>::SampleDataType, WaveGeneratorGetter<INT24>::SampleDataTypeName, PcmData<INT24>::FormatTag, sizeof(INT24) * 8 },
	{ WaveGeneratorGetter<float>::SampleDataType, WaveGeneratorGetter<float>::SampleDataTypeName, PcmData<float>::FormatTag, sizeof(float) * 8 },
};

static const PcmDataEnumerator::WaveGeneratorProperty waveGeneratorProperties[] = {
	{ IPcmData::WaveGeneratorType::SquareWave, IWaveGenerator::SquareWaveForm, createSquareWaveGenerator, PcmDataEnumerator::FactoryParameter::Duty },
	{ IPcmData::WaveGeneratorType::SineWave, IWaveGenerator::SineWaveForm, [](IPcmData::SampleDataType type, float param) { return createSineWaveGenerator(type); }, PcmDataEnumerator::FactoryParameter::None },
	{ IPcmData::WaveGeneratorType::TriangleWave, IWaveGenerator::TriangleWaveForm, createTriangleWaveGenerator, PcmDataEnumerator::FactoryParameter::PeakPosition },
};

/*static*/ const std::vector<PcmDataEnumerator::SampleDataTypeProperty>& PcmDataEnumerator::getSampleDatatypeProperties()
{
	auto& ar(sampleDataTypeProperties);
	static std::vector<SampleDataTypeProperty> ret(&ar[0], &ar[ARRAYSIZE(ar)]);
	return ret;
}

/*static*/ const std::vector<PcmDataEnumerator::WaveGeneratorProperty>& PcmDataEnumerator::getWaveGeneratorProperties()
{
	auto& ar(waveGeneratorProperties);
	static std::vector<WaveGeneratorProperty> ret(&ar[0], &ar[ARRAYSIZE(ar)]);
	return ret;
}
#pragma endregion

IPcmData* createPcmData(WORD samplesPerSec, WORD channels, IWaveGenerator* waveGenerator)
{
	if(!waveGenerator) { return nullptr; }

	switch(waveGenerator->getSampleDataType()) {
	case IPcmData::SampleDataType::PCM_8bits:
		return new PcmData<UINT8>(samplesPerSec, channels, waveGenerator);
	case IPcmData::SampleDataType::PCM_16bits:
		return new PcmData<INT16>(samplesPerSec, channels, waveGenerator);
	case IPcmData::SampleDataType::PCM_24bits:
		return new PcmData<INT24>(samplesPerSec, channels, waveGenerator);
	case IPcmData::SampleDataType::IEEE_Float:
		return new PcmData<float>(samplesPerSec, channels, waveGenerator);
	default:
		return nullptr;
	}
}

IWaveGenerator* createSquareWaveGenerator(IPcmData::SampleDataType sampleDataType, float duty)
{
	switch(sampleDataType) {
	case IPcmData::SampleDataType::PCM_8bits:
		return new SquareWaveGenerator<UINT8>(duty);
	case IPcmData::SampleDataType::PCM_16bits:
		return new SquareWaveGenerator<INT16>(duty);
	case IPcmData::SampleDataType::PCM_24bits:
		return new SquareWaveGenerator<INT24>(duty);
	case IPcmData::SampleDataType::IEEE_Float:
		return new SquareWaveGenerator<float>(duty);
	default:
		return nullptr;
	}
}

IWaveGenerator* createSineWaveGenerator(IPcmData::SampleDataType sampleDataType)
{
	switch(sampleDataType) {
	case IPcmData::SampleDataType::PCM_8bits:
		return new SineWaveGenerator<UINT8>();
	case IPcmData::SampleDataType::PCM_16bits:
		return new SineWaveGenerator<INT16>();
	case IPcmData::SampleDataType::PCM_24bits:
		return new SineWaveGenerator<INT24>();
	case IPcmData::SampleDataType::IEEE_Float:
		return new SineWaveGenerator<float>();
	default:
		return nullptr;
	}
}

IWaveGenerator* createTriangleWaveGenerator(IPcmData::SampleDataType sampleDataType, float peakPosition)
{
	switch(sampleDataType) {
	case IPcmData::SampleDataType::PCM_8bits:
		return new TriangleWaveGenerator<UINT8>(peakPosition);
	case IPcmData::SampleDataType::PCM_16bits:
		return new TriangleWaveGenerator<INT16>(peakPosition);
	case IPcmData::SampleDataType::PCM_24bits:
		return new TriangleWaveGenerator<INT24>(peakPosition);
	case IPcmData::SampleDataType::IEEE_Float:
		return new TriangleWaveGenerator<float>(peakPosition);
	default:
		return nullptr;
	}
}
