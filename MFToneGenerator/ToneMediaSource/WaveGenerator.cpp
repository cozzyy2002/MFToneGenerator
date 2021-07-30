#include "pch.h"

#include "WaveGenerator.h"


#pragma region Declaration for available T types.

template<> const UINT16 WaveGenerator<UINT16>::HighValue = 2000;
template<> const UINT16 WaveGenerator<UINT16>::LowValue = 0;

template<> const UINT8 WaveGenerator<UINT8>::HighValue = 200;
template<> const UINT8 WaveGenerator<UINT8>::LowValue = 0;

#pragma endregion

/*static*/ const float IWaveGenerator::DefaultLevel = 0.2f;

#if 0
static void test()
{
	Square8bpsWaveGenerator gen(44100, 2);
	if(!gen) {
		gen.generate(440);
	}
	IMFMediaBuffer* dest = NULL;
	gen.copyTo(&dest, 200);
}
#endif