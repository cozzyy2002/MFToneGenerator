#include "pch.h"

#include "WaveGenerator.h"


#pragma region Declaration for available T types.

template<>
UINT16 WaveGenerator<UINT16>::getHighValue() const { return 2000; }
template<>
UINT16 WaveGenerator<UINT16>::getLowValue() const { return 0; }

template<>
UINT8 WaveGenerator<UINT8>::getHighValue() const { return 200; }
template<>
UINT8 WaveGenerator<UINT8>::getLowValue() const { return 0; }

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