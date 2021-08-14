// PcmDataTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <PcmData.h>
#include <PcmDataImpl.h>

#include <vector>
#include <iostream>

class ITestPcmData : public IUnknown
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
	TestPcmData(WORD samplesPerSecond, WORD channels, IWaveGenerator* waveGenerator)
		: PcmData<T>(samplesPerSecond, channels, waveGenerator) {}

	virtual IPcmData* getPcmData() override { return this; }
	virtual std::string str(size_t pos) override;

	virtual HRESULT STDMETHODCALLTYPE IUnknown::QueryInterface(REFIID riid, void** ppvObject) override { return PcmData<T>::QueryInterface(riid, ppvObject); }
	virtual ULONG STDMETHODCALLTYPE IUnknown::AddRef(void) override { return PcmData<T>::AddRef(); }
	virtual ULONG STDMETHODCALLTYPE IUnknown::Release() override { return PcmData<T>::Release(); }
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
	auto peakPosition = 0.25f;
	WORD samplesPerSecond = 44100;
	WORD channels = 1;
	WORD key = 440;
	float level = 1.0f;
	float phaseShift = 0;

	float fVal;
	int iVal;
	for(int i = 1; i < argc; i++) {
		if(sscanf_s(argv[i], "duty=%f", &fVal) == 1) { duty = fVal; continue; }
		if(sscanf_s(argv[i], "peak=%f", &fVal) == 1) { peakPosition = fVal; continue; }
		if(sscanf_s(argv[i], "sps=%d", &iVal) == 1) { samplesPerSecond = iVal; continue; }
		if(sscanf_s(argv[i], "ch=%d", &iVal) == 1) { channels = iVal; continue; }
		if(sscanf_s(argv[i], "key=%d", &iVal) == 1) { key = iVal; continue; }
		if(sscanf_s(argv[i], "lvl=%f", &fVal) == 1) { level = fVal; continue; }
		if(sscanf_s(argv[i], "sft=%f", &fVal) == 1) { phaseShift = fVal; continue; }

		std::cout << "Usage: PcmDataTest [duty=Duty] [peak=PeakPosition] [sps=SamplesPerSecond] [ch=Channels] [key=Key] [lvl=Level] [sft=PhaseSift]\n";
		return 0;
	}

	std::cout << "Command Line Parameters"
		<< "\nDuty," << duty
		<< "\nPeak Position," << peakPosition
		<< "\nSamples Per Second," << samplesPerSecond
		<< "\nChannels," << channels
		<< "\nKey," << key
		<< "\nLevel," << level
		<< "\nPhase Shift," << phaseShift
		<< "\n\n";

	// Create PcmData objects associate with all kinds of WaveGenerator.
	std::vector<CComPtr<ITestPcmData>> pcmDataList;
	pcmDataList.push_back(new TestPcmData<UINT8>(samplesPerSecond, channels, new SquareWaveGenerator<UINT8>(duty)));
	pcmDataList.push_back(new TestPcmData<UINT8>(samplesPerSecond, channels, new SineWaveGenerator<UINT8>()));
	pcmDataList.push_back(new TestPcmData<UINT8>(samplesPerSecond, channels, new TriangleWaveGenerator<UINT8>(peakPosition)));
	pcmDataList.push_back(new TestPcmData<INT16>(samplesPerSecond, channels, new SquareWaveGenerator<INT16>(duty)));
	pcmDataList.push_back(new TestPcmData<INT16>(samplesPerSecond, channels, new SineWaveGenerator<INT16>()));
	pcmDataList.push_back(new TestPcmData<INT16>(samplesPerSecond, channels, new TriangleWaveGenerator<INT16>(peakPosition)));
	pcmDataList.push_back(new TestPcmData<float>(samplesPerSecond, channels, new SquareWaveGenerator<float>(duty)));
	pcmDataList.push_back(new TestPcmData<float>(samplesPerSecond, channels, new SineWaveGenerator<float>()));
	pcmDataList.push_back(new TestPcmData<float>(samplesPerSecond, channels, new TriangleWaveGenerator<float>(peakPosition)));

	// Generate PCM data and show properties of each PcmData object.
	std::cout << ",Wave form,Sample type,Bits per sample,Channels,Block align,Samples in cycle,Byte size of cycle\n";
	int i = 0;
	for(auto& x : pcmDataList) {
		auto pcmData = x->getPcmData();
		pcmData->generate(key, level, phaseShift);

		std::cout << i++
			<< "," << pcmData->getWaveForm()
			<< "," << pcmData->getSampleTypeName()
			<< "," << pcmData->getBitsPerSample()
			<< "," << pcmData->getChannels()
			<< "," << pcmData->getBlockAlign()
			<< "," << pcmData->getSamplesPerCycle()
			<< "," << pcmData->getBlockAlign() * pcmData->getSamplesPerCycle()
			<< std::endl;
	}
	std::cout << std::endl;

	auto sampleCountInCycle = pcmDataList[0]->getPcmData()->getSamplesPerCycle();
	auto comma = std::string(",,,,").substr(0, channels - 1);
	std::cout << ",UINT8" << comma << "," << comma << "," << comma << ",INT16" << comma << "," << comma << "," << comma << ",float\n";
	std::cout << "pos,Square" << comma << ",Sine" << comma << ",Triangle" << comma
				<< ",Square" << comma << ",Sine" << comma << ",Triangle" << comma
				<< ",Square" << comma << ",Sine" << comma << ",Triangle\n";
	for(size_t pos = 0; pos < sampleCountInCycle; pos += channels) {
		std::cout << (pos / channels);
		for(auto& x : pcmDataList) {
			for(WORD ch = 0; ch < channels; ch++) {
				std::cout << "," << x->str(pos + ch);
			}
		}
		std::cout << std::endl;
	}

	return 0;
}


/*static*/ HRESULT tsm::Assert::checkHResult(HRESULT hr, LPCTSTR exp, LPCTSTR sourceFile, int line)
{
	return hr;
}
