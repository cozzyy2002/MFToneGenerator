// PcmDataTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <PcmData/PcmData.h>
#include <PcmData/INT24.h>

#include <vector>
#include <iostream>

// template function to be called by handler::str() method.
// Returns string converted from type T number value.
// Parameter pos must be position of type T not BYTE.
template<typename T>
std::string str_func(LPCVOID cycleData, size_t pos);

template<typename T> 
std::string str_func<T>(LPCVOID cycleData, size_t pos)
{
	char s[30];
	_itoa_s(((T*)cycleData)[pos], s, 10);
	return s;
}

template<>
std::string str_func<float>(LPCVOID cycleData, size_t pos)
{	char s[20];
	_gcvt_s(s, ((float*)cycleData)[pos], 8);
	return s;
}

class Handler {
public:
	Handler(IPcmData* pcmData) : m_pcmData(pcmData)
	{
		switch(pcmData->getSampleDataType())
		{
		case IPcmData::SampleDataType::PCM_8bits: m_str_func = str_func<UINT8>; break;
		case IPcmData::SampleDataType::PCM_16bits: m_str_func = str_func<INT16>; break;
		case IPcmData::SampleDataType::PCM_24bits: m_str_func = str_func<INT24>; break;
		case IPcmData::SampleDataType::IEEE_Float: m_str_func = str_func<float>; break;
		default:
			m_str_func = [](LPCVOID cycleData, size_t pos) { return std::string("?type?"); };
			break;
		}
	}

	void generate(float key, float level, float phaseShift)
	{
		m_pcmData->generate(key, level, phaseShift);
		auto cycleDataSize = m_pcmData->getSamplesPerCycle() * m_pcmData->getBlockAlign();
		m_cycleData = std::make_unique<BYTE[]>(cycleDataSize);
		m_pcmData->copyTo(m_cycleData.get(), cycleDataSize);
	}

	std::string str(size_t pos)
	{
		return m_cycleData ? m_str_func(m_cycleData.get(), pos) : "?null?";
	}

	IPcmData* getPcmData() { return m_pcmData.get(); }

protected:
	std::unique_ptr<IPcmData> m_pcmData;
	std::unique_ptr<BYTE[]> m_cycleData;
	using str_func_t = std::string (*)(LPCVOID cycleData, size_t pos);
	str_func_t m_str_func;
};

int main(int argc, char* argv[])
{
	auto duty = 0.5f;
	auto peakPosition = 0.5f;
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
	IWaveGenerator* waveGenerators[] = {
		createSquareWaveGenerator(IPcmData::SampleDataType::PCM_8bits, duty),
		createSineWaveGenerator(IPcmData::SampleDataType::PCM_8bits),
		createTriangleWaveGenerator(IPcmData::SampleDataType::PCM_8bits, peakPosition),
		createSquareWaveGenerator(IPcmData::SampleDataType::PCM_16bits, duty),
		createSineWaveGenerator(IPcmData::SampleDataType::PCM_16bits),
		createTriangleWaveGenerator(IPcmData::SampleDataType::PCM_16bits, peakPosition),
		createSquareWaveGenerator(IPcmData::SampleDataType::PCM_24bits, duty),
		createSineWaveGenerator(IPcmData::SampleDataType::PCM_24bits),
		createTriangleWaveGenerator(IPcmData::SampleDataType::PCM_24bits, peakPosition),
		createSquareWaveGenerator(IPcmData::SampleDataType::IEEE_Float, duty),
		createSineWaveGenerator(IPcmData::SampleDataType::IEEE_Float),
		createTriangleWaveGenerator(IPcmData::SampleDataType::IEEE_Float, peakPosition),
	};
	std::vector<std::unique_ptr<Handler>> handlers;
	for(auto gen : waveGenerators) {
		handlers.push_back(std::make_unique<Handler>(createPcmData(samplesPerSecond, channels, gen)));
	}

	// Generate PCM data and show properties of each PcmData object.
	std::cout << ",Wave form,Sample type,Bits per sample,Channels,Block align,Samples in cycle,Byte size of cycle\n";
	int i = 0;
	for(auto& handler : handlers) {
		handler->generate(key, level, phaseShift);

		auto pcmData = handler->getPcmData();
		std::cout << i++
			<< "," << pcmData->getWaveForm()
			<< "," << pcmData->getSampleTypeName()
			<< "," << pcmData->getBitsPerSample()
			<< "," << pcmData->getChannels()
			<< "," << pcmData->getBlockAlign()
			<< "," << pcmData->getSamplesPerCycle()
			<< "," << pcmData->getSamplesPerCycle() * pcmData->getBlockAlign()
			<< std::endl;
	}
	std::cout << std::endl;

	auto sampleCountInCycle = handlers[0]->getPcmData()->getSamplesPerCycle();
	auto comma = std::string(",,,,").substr(0, channels - 1);
	std::cout << ",UINT8" << comma << "," << comma << "," << comma << ",INT16" << comma << "," << comma << "," << comma << ",float\n";
	std::cout << "pos,Square" << comma << ",Sine" << comma << ",Triangle" << comma
				<< ",Square" << comma << ",Sine" << comma << ",Triangle" << comma
				<< ",Square" << comma << ",Sine" << comma << ",Triangle" << comma
				<< ",Square" << comma << ",Sine" << comma << ",Triangle\n";
	for(size_t pos = 0; pos < sampleCountInCycle; pos += channels) {
		std::cout << (pos / channels);
		for(auto& handler : handlers) {
			for(WORD ch = 0; ch < channels; ch++) {
				std::cout << "," << handler->str(pos + ch);
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
