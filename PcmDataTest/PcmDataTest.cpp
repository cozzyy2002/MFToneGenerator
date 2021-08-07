// PcmDataTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <PcmData.h>
#include <PcmDataImpl.h>

#include <iostream>

template<typename T>
class TestPcmData : public PcmData<T>
{
public:
	TestPcmData(WORD samplesPerSec, IWaveGenerator* waveGenerator)
		: PcmData<T>(samplesPerSec, 1 /*channels*/, waveGenerator) {}

	std::string str(size_t pos);
	using PcmData<T>::m_cycleData;
	using PcmData<T>::m_cycleSize;
};

template<typename T> 
std::string TestPcmData<T>::str(size_t pos)
{
	char s[30];
	_itoa_s(m_cycleData[pos], s, 10);
	return s;
}

template<>
std::string TestPcmData<float>::str(size_t pos)
{
	char s[20];
	_gcvt_s(s, m_cycleData[pos], 8);
	return s;
}

int main()
{
	using SampleType = float;
	static const DWORD SamplePerSecond = 44100;
	static const WORD key = 440;

	CComPtr<TestPcmData<SampleType>> squareData(new TestPcmData<SampleType>(SamplePerSecond, new SquareWaveGenerator<SampleType>(0.5)));
	squareData->generate(key, 1);
	CComPtr<TestPcmData<SampleType>> sineData(new TestPcmData<SampleType>(SamplePerSecond, new SineWaveGenerator<SampleType>()));
	sineData->generate(key, 1);

	std::cout << "pos\tSquare\tSine\n";
	for(size_t pos = 0; pos < sineData->m_cycleSize; pos++) {
		std::cout << pos
			<< "\t" << squareData->str(pos)
			<< "\t" << sineData->str(pos)
			<< std::endl;
	}

	return 0;
}


/*static*/ HRESULT tsm::Assert::checkHResult(HRESULT hr, LPCTSTR exp, LPCTSTR sourceFile, int line)
{
	return hr;
}
