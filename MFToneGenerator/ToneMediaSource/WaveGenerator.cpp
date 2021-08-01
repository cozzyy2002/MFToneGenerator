#include "pch.h"

#include "WaveGenerator.h"


#pragma region Declaration for available T types.

template<> const WORD WaveGenerator<INT16>::FormatTag = WAVE_FORMAT_PCM;
template<> const INT16 WaveGenerator<INT16>::HighValue = 8000;
template<> const INT16 WaveGenerator<INT16>::ZeroValue = 0;
template<> const INT16 WaveGenerator<INT16>::LowValue = -8000;

template<> const WORD WaveGenerator<UINT8>::FormatTag = WAVE_FORMAT_PCM;
template<> const UINT8 WaveGenerator<UINT8>::HighValue = 0xc0;
template<> const UINT8 WaveGenerator<UINT8>::ZeroValue = 0x80;
template<> const UINT8 WaveGenerator<UINT8>::LowValue = 0x40;

template<> const WORD WaveGenerator<float>::FormatTag = WAVE_FORMAT_IEEE_FLOAT;
template<> const float WaveGenerator<float>::HighValue = 0.5f;
template<> const float WaveGenerator<float>::ZeroValue = 0.0f;
template<> const float WaveGenerator<float>::LowValue = -0.5f;

#pragma endregion

/*static*/ const float IWaveGenerator::DefaultLevel = 0.2f;
