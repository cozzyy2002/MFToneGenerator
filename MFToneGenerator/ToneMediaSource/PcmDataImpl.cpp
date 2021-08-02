#include "pch.h"

#include "PcmDataImpl.h"


#pragma region Declaration for available T types.

template<> const IPcmData::SampleDataType WaveGenerator<INT16>::SampleDataType = IPcmData::SampleDataType::_16bits;
template<> const IPcmData::SampleDataType WaveGenerator<UINT8>::SampleDataType = IPcmData::SampleDataType::_8bits;
template<> const IPcmData::SampleDataType WaveGenerator<float>::SampleDataType = IPcmData::SampleDataType::IEEE_Float;

template<> const WORD PcmData<INT16>::FormatTag = WAVE_FORMAT_PCM;
template<> const INT16 PcmData<INT16>::HighValue = 8000;
template<> const INT16 PcmData<INT16>::ZeroValue = 0;
template<> const INT16 PcmData<INT16>::LowValue = -8000;

template<> const WORD PcmData<UINT8>::FormatTag = WAVE_FORMAT_PCM;
template<> const UINT8 PcmData<UINT8>::HighValue = 0xc0;
template<> const UINT8 PcmData<UINT8>::ZeroValue = 0x80;
template<> const UINT8 PcmData<UINT8>::LowValue = 0x40;

template<> const WORD PcmData<float>::FormatTag = WAVE_FORMAT_IEEE_FLOAT;
template<> const float PcmData<float>::HighValue = 0.5f;
template<> const float PcmData<float>::ZeroValue = 0.0f;
template<> const float PcmData<float>::LowValue = -0.5f;

#pragma endregion

/*static*/ IPcmData* IPcmData::create(WORD samplesPerSec, WORD channels, IWaveGenerator* waveGenerator)
{
	if(!waveGenerator) { return nullptr; }

	switch(waveGenerator->getSampleDatatype()) {
	case SampleDataType::_8bits:
		return new PcmData<UINT8>(samplesPerSec, channels, (WaveGenerator<UINT8>*)waveGenerator);
	case SampleDataType::_16bits:
		return new PcmData<INT16>(samplesPerSec, channels, (WaveGenerator<INT16>*)waveGenerator);
	case SampleDataType::IEEE_Float:
		return new PcmData<float>(samplesPerSec, channels, (WaveGenerator<float>*)waveGenerator);
	default:
		return nullptr;
	}
}

/*static*/ IWaveGenerator* IPcmData::createSquareWaveGenerator(IPcmData::SampleDataType sampleDataType, float duty)
{
	switch(sampleDataType) {
	case IPcmData::SampleDataType::_8bits:
		return new SquareWaveGenerator<UINT8>(duty);
	case IPcmData::SampleDataType::_16bits:
		return new SquareWaveGenerator<INT16>(duty);
	case IPcmData::SampleDataType::IEEE_Float:
		return new SquareWaveGenerator<float>(duty);
	default:
		return nullptr;
	}
}

/*static*/ IWaveGenerator* IPcmData::createSineWaveGenerator(IPcmData::SampleDataType sampleDataType)
{
	switch(sampleDataType) {
	case IPcmData::SampleDataType::_8bits:
		return new SineWaveGenerator<UINT8>();
	case IPcmData::SampleDataType::_16bits:
		return new SineWaveGenerator<INT16>();
	case IPcmData::SampleDataType::IEEE_Float:
		return new SineWaveGenerator<float>();
	default:
		return nullptr;
	}
}
