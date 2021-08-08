// PcmDataTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <PcmData.h>
#include <PcmDataImpl.h>

#include <vector>
#include <iostream>

class ITestPcmData
{
public:
	virtual ~ITestPcmData() {}

	virtual void generate() = 0;
	virtual size_t getCycleSize() = 0;
	virtual std::string str(size_t pos) = 0;
};

template<typename T>
class TestPcmData : public ITestPcmData, public PcmData<T>
{
public:
	TestPcmData(IWaveGenerator* waveGenerator)
		: PcmData<T>(SamplesPerSecond, 1 /*channels*/, waveGenerator) {}

	virtual void generate() override { PcmData<T>::generate(Key, 1 /*level*/); }
	virtual size_t getCycleSize() override { return PcmData<T>::m_cycleSize; }
	virtual std::string str(size_t pos) override;

	static const WORD SamplesPerSecond = 44100;
	static const WORD Key = 440;
};

template<typename T> 
std::string TestPcmData<T>::str(size_t pos)
{
	char s[30];
	_itoa_s(PcmData<T>::m_cycleData[pos], s, 10);
	return s;
}

template<>
std::string TestPcmData<float>::str(size_t pos)
{	char s[20];
	_gcvt_s(s, PcmData<float>::m_cycleData[pos], 8);
	return s;
}

int main()
{
	static const auto duty = 0.5f;
	static const auto peakPosition = 0.5f;

	std::vector<std::unique_ptr<ITestPcmData>> pcmDataList;
	pcmDataList.push_back(std::make_unique<TestPcmData<UINT8>>(new SquareWaveGenerator<UINT8>(duty)));
	pcmDataList.push_back(std::make_unique<TestPcmData<UINT8>>(new SineWaveGenerator<UINT8>()));
	pcmDataList.push_back(std::make_unique<TestPcmData<UINT8>>(new TriangleWaveGenerator<UINT8>(peakPosition)));
	pcmDataList.push_back(std::make_unique<TestPcmData<INT16>>(new SquareWaveGenerator<INT16>(duty)));
	pcmDataList.push_back(std::make_unique<TestPcmData<INT16>>(new SineWaveGenerator<INT16>()));
	pcmDataList.push_back(std::make_unique<TestPcmData<INT16>>(new TriangleWaveGenerator<INT16>(peakPosition)));
	pcmDataList.push_back(std::make_unique<TestPcmData<float>>(new SquareWaveGenerator<float>(duty)));
	pcmDataList.push_back(std::make_unique<TestPcmData<float>>(new SineWaveGenerator<float>()));
	pcmDataList.push_back(std::make_unique<TestPcmData<float>>(new TriangleWaveGenerator<float>(peakPosition)));
	for(auto& x : pcmDataList) {
		x->generate();
	}

	auto cycleSize = pcmDataList[0]->getCycleSize();
	std::cout << ",UINT8,,,INT16,,,float\n";
	std::cout << "pos,Square,Sine,Triangle,Square,Sine,Triangle,Square,Sine,Triangle\n";
	for(size_t pos = 0; pos < cycleSize; pos++) {
		std::cout << pos;
		for(auto& x : pcmDataList) {
			std::cout << "," << x->str(pos);
		}
		std::cout << std::endl;
	}

	return 0;
}


/*static*/ HRESULT tsm::Assert::checkHResult(HRESULT hr, LPCTSTR exp, LPCTSTR sourceFile, int line)
{
	return hr;
}
