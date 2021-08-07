// PcmDataTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <PcmData.h>
#include <PcmDataImpl.h>

#include <vector>
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
	static const WORD SamplePerSecond = 44100;
	static const WORD key = 440;

	std::vector<std::unique_ptr<TestPcmData<SampleType>>> pcmDataList;
	pcmDataList.push_back(std::make_unique<TestPcmData<SampleType>>(SamplePerSecond, new SquareWaveGenerator<SampleType>(0.5)));
	pcmDataList.push_back(std::make_unique<TestPcmData<SampleType>>(SamplePerSecond, new SineWaveGenerator<SampleType>()));
	pcmDataList.push_back(std::make_unique<TestPcmData<SampleType>>(SamplePerSecond, new TriangleWaveGenerator<SampleType>(0.5)));
	for(auto& x : pcmDataList) {
		x->generate(key, 1);
	}

	auto cycleSize = pcmDataList[0]->m_cycleSize;
	std::cout << "pos\tSquare\tSine\tTriangle\n";
	for(size_t pos = 0; pos < cycleSize; pos++) {
		std::cout << pos;
		for(auto& x : pcmDataList) {
			std::cout << "\t" << x->str(pos);
		}
		std::cout << std::endl;
	}

	return 0;
}


/*static*/ HRESULT tsm::Assert::checkHResult(HRESULT hr, LPCTSTR exp, LPCTSTR sourceFile, int line)
{
	return hr;
}
