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

const char* IWaveGenerator::SquareWaveFormTypeName = "Square Wave";
const char* IWaveGenerator::SineWaveFormTypeName = "Sine Wave";
const char* IWaveGenerator::TriangleWaveFormTypeName = "Triangle Wave";

template<> const WORD PcmData<UINT8>::FormatTag = WAVE_FORMAT_PCM;
template<> const UINT8 PcmData<UINT8>::HighValue = 0xc0;
template<> const UINT8 PcmData<UINT8>::ZeroValue = 0x80;
template<> const UINT8 PcmData<UINT8>::LowValue = 0x40;

template<> const WORD PcmData<INT16>::FormatTag = WAVE_FORMAT_PCM;
template<> const INT16 PcmData<INT16>::HighValue = 0x6000;
template<> const INT16 PcmData<INT16>::ZeroValue = 0;
template<> const INT16 PcmData<INT16>::LowValue = -0x6000;

template<> const WORD PcmData<INT24>::FormatTag = WAVE_FORMAT_PCM;
template<> const INT24 PcmData<INT24>::HighValue = 0x600000;
template<> const INT24 PcmData<INT24>::ZeroValue = 0;
template<> const INT24 PcmData<INT24>::LowValue = -0x600000;

template<> const WORD PcmData<float>::FormatTag = WAVE_FORMAT_IEEE_FLOAT;
template<> const float PcmData<float>::HighValue = 0.8f;
template<> const float PcmData<float>::ZeroValue = 0.0f;
template<> const float PcmData<float>::LowValue = -0.8f;

#pragma endregion

#pragma region Implementation of PcmDataEnumerator

/*static*/ const float PcmDataEnumerator::DefaultDuty = 0.5f;
/*static*/ const float PcmDataEnumerator::DefaultPeakPosition = 0.25f;

static const PcmDataEnumerator::SampleDataTypeProperty sampleDataTypeProperties[] = {
	{ WaveGenerator<UINT8>::SampleDataType, WaveGenerator<UINT8>::SampleDataTypeName, PcmData<UINT8>::FormatTag, sizeof(UINT8) * 8 },
	{ WaveGenerator<INT16>::SampleDataType, WaveGenerator<INT16>::SampleDataTypeName, PcmData<INT16>::FormatTag, sizeof(INT16) * 8 },
	{ WaveGenerator<INT24>::SampleDataType, WaveGenerator<INT24>::SampleDataTypeName, PcmData<INT24>::FormatTag, sizeof(INT24) * 8 },
	{ WaveGenerator<float>::SampleDataType, WaveGenerator<float>::SampleDataTypeName, PcmData<float>::FormatTag, sizeof(float) * 8 },
};

static const PcmDataEnumerator::WaveFormProperty waveGeneratorProperties[] = {
	{ IPcmData::WaveFormType::SquareWave, IWaveGenerator::SquareWaveFormTypeName, createSquareWaveGenerator, PcmDataEnumerator::FactoryParameter::Duty, PcmDataEnumerator::DefaultDuty },
	{ IPcmData::WaveFormType::SineWave, IWaveGenerator::SineWaveFormTypeName, createSineWaveGenerator, PcmDataEnumerator::FactoryParameter::None },
	{ IPcmData::WaveFormType::TriangleWave, IWaveGenerator::TriangleWaveFormTypeName, createTriangleWaveGenerator, PcmDataEnumerator::FactoryParameter::PeakPosition, PcmDataEnumerator::DefaultPeakPosition },
};

/*static*/ const std::vector<PcmDataEnumerator::SampleDataTypeProperty>& PcmDataEnumerator::getSampleDatatypeProperties()
{
	auto& ar(sampleDataTypeProperties);
	static std::vector<SampleDataTypeProperty> ret(&ar[0], &ar[ARRAYSIZE(ar)]);
	return ret;
}

/*static*/ const std::vector<PcmDataEnumerator::WaveFormProperty>& PcmDataEnumerator::getWaveFormProperties()
{
	auto& ar(waveGeneratorProperties);
	static std::vector<WaveFormProperty> ret(&ar[0], &ar[ARRAYSIZE(ar)]);
	return ret;
}

/*static*/ const PcmDataEnumerator::SampleDataTypeProperty& PcmDataEnumerator::getSampleDataTypeProperty(IPcmData::SampleDataType type)
{
	for(auto& sp : getSampleDatatypeProperties()) {
		if(sp.type == type) return sp;
	}

	static const SampleDataTypeProperty unknown = {
		IPcmData::SampleDataType::Unknown,
		"UnknownSampleDataType"
	};
	return unknown;
}

/*static*/ const PcmDataEnumerator::WaveFormProperty& PcmDataEnumerator::getWaveFormProperty(IPcmData::WaveFormType type)
{
	for(auto& wp : getWaveFormProperties()) {
		if(wp.type == type) return wp;
	}

	static const WaveFormProperty unknown = {
		IPcmData::WaveFormType::Unknown,
		"UnknownWaveForm",
		[](IPcmData::SampleDataType, float) { return (IWaveGenerator*)nullptr; }
	};
	return unknown;
}

#pragma endregion

std::shared_ptr <IPcmData> createPcmData(DWORD samplesPerSec, WORD channels, IWaveGenerator* waveGenerator)
{
	if(!waveGenerator) { return nullptr; }

	IPcmData* p = nullptr;
	switch(waveGenerator->getSampleDataType()) {
	case IPcmData::SampleDataType::PCM_8bits:
		p = new PcmData<UINT8>(samplesPerSec, channels, waveGenerator);
		break;
	case IPcmData::SampleDataType::PCM_16bits:
		p = new PcmData<INT16>(samplesPerSec, channels, waveGenerator);
		break;
	case IPcmData::SampleDataType::PCM_24bits:
		p = new PcmData<INT24>(samplesPerSec, channels, waveGenerator);
		break;
	case IPcmData::SampleDataType::IEEE_Float:
		p = new PcmData<float>(samplesPerSec, channels, waveGenerator);
		break;
	}
	return std::shared_ptr<IPcmData>(p);
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

IWaveGenerator* createSineWaveGenerator(IPcmData::SampleDataType sampleDataType, float)
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
