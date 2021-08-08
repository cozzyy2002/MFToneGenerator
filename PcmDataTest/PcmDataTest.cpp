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

	virtual IPcmData* getPcmData() = 0;
	virtual std::string str(size_t pos) = 0;
};

template<typename T>
class TestPcmData : public ITestPcmData, public PcmData<T>
{
public:
	TestPcmData(WORD samplesPerSecond, IWaveGenerator* waveGenerator)
		: PcmData<T>(samplesPerSecond, 1 /*channels*/, waveGenerator) {}

	virtual IPcmData* getPcmData() override { return this; }
	virtual std::string str(size_t pos) override;

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

int main(int argc, char* argv[])
{
	auto duty = 0.5f;
	auto peakPosition = 0.5f;
	WORD samplesPerSecond = 44100;
	WORD key = 440;

	float fVal;
	int iVal;
	for(int i = 1; i < argc; i++) {
		if(sscanf_s(argv[i], "duty=%f", &fVal) == 1) { duty = fVal; continue; }
		if(sscanf_s(argv[i], "peak=%f", &fVal) == 1) { peakPosition = fVal; continue; }
		if(sscanf_s(argv[i], "sps=%d", &iVal) == 1) { samplesPerSecond = iVal; continue; }
		if(sscanf_s(argv[i], "key=%d", &iVal) == 1) { key = iVal; continue; }

		printf_s("Usage: PcmDataTest [duty=Duty] [peak=PeakPosition] [sps=SamplesPerSecond] [key=Key]\n");
		return 0;
	}

	std::vector<std::unique_ptr<ITestPcmData>> pcmDataList;
	pcmDataList.push_back(std::make_unique<TestPcmData<UINT8>>(samplesPerSecond, new SquareWaveGenerator<UINT8>(duty)));
	pcmDataList.push_back(std::make_unique<TestPcmData<UINT8>>(samplesPerSecond, new SineWaveGenerator<UINT8>()));
	pcmDataList.push_back(std::make_unique<TestPcmData<UINT8>>(samplesPerSecond, new TriangleWaveGenerator<UINT8>(peakPosition)));
	pcmDataList.push_back(std::make_unique<TestPcmData<INT16>>(samplesPerSecond, new SquareWaveGenerator<INT16>(duty)));
	pcmDataList.push_back(std::make_unique<TestPcmData<INT16>>(samplesPerSecond, new SineWaveGenerator<INT16>()));
	pcmDataList.push_back(std::make_unique<TestPcmData<INT16>>(samplesPerSecond, new TriangleWaveGenerator<INT16>(peakPosition)));
	pcmDataList.push_back(std::make_unique<TestPcmData<float>>(samplesPerSecond, new SquareWaveGenerator<float>(duty)));
	pcmDataList.push_back(std::make_unique<TestPcmData<float>>(samplesPerSecond, new SineWaveGenerator<float>()));
	pcmDataList.push_back(std::make_unique<TestPcmData<float>>(samplesPerSecond, new TriangleWaveGenerator<float>(peakPosition)));
	for(auto& x : pcmDataList) {
		x->getPcmData()->generate(key, 1 /*level*/);
	}

	auto sampleCountInCycle = pcmDataList[0]->getPcmData()->getSampleCountInCycle();
	std::cout << ",UINT8,,,INT16,,,float\n";
	std::cout << "pos,Square,Sine,Triangle,Square,Sine,Triangle,Square,Sine,Triangle\n";
	for(size_t pos = 0; pos < sampleCountInCycle; pos++) {
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
