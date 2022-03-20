// PcmDataTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <PcmData/PcmData.h>
#include <PcmData/PcmSample.h>

#include <vector>
#include <iostream>

int main(int argc, char* argv[])
{
	auto duty = 0.5f;
	auto peakPosition = 0.5f;
	DWORD samplesPerSecond = 44100;
	WORD channels = 1;
	WORD key = 440;
	float level = 1.0f;
	float phaseShift = 0;

	auto& sampleDataTypeProperties(PcmDataEnumerator::getSampleDatatypeProperties());
	auto& waveFormProperties(PcmDataEnumerator::getWaveFormProperties());

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

		std::cerr << "Usage: PcmDataTest [duty=Duty] [peak=PeakPosition] [sps=SamplesPerSecond] [ch=Channels] [key=Key] [lvl=Level] [sft=PhaseSift]\n";
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

	// Create PcmData objects and it's Handler objects associate with all combination of SampleDataType and WaveForm.
	std::vector<std::unique_ptr<IPcmSample>> pcmSamples;
	for(auto& sp : sampleDataTypeProperties) {
		for(auto& wp : waveFormProperties) {
			float param = 0.0f;
			switch(wp.parameter) {
			case PcmDataEnumerator::FactoryParameter::Duty:
				param = duty;
				break;
			case PcmDataEnumerator::FactoryParameter::PeakPosition:
				param = peakPosition;
				break;
			default:
				break;
			}
			auto gen = wp.factory(sp.type, param);
			auto pcmData = createPcmData(samplesPerSecond, channels, gen);
			pcmData->generate(key, level, phaseShift);
			pcmSamples.push_back(std::unique_ptr<IPcmSample>(createPcmSample(pcmData)));
		}
	}

	// Generate PCM data and show properties of each PcmData object.
	std::cout << ",Wave form,Sample type,Bits per sample,Channels,Block align,Samples in cycle,Byte size of cycle, High value, Zero value, Low value\n";
	int i = 0;
	for(auto& pcmSample : pcmSamples) {
		auto pcmData = pcmSample->getPcmData();
		std::cout << i++
			<< "," << pcmData->getWaveFormTypeName()
			<< "," << pcmData->getSampleTypeName()
			<< "," << pcmData->getBitsPerSample()
			<< "," << pcmData->getChannels()
			<< "," << pcmData->getBlockAlign()
			<< "," << pcmData->getSamplesPerCycle()
			<< "," << pcmData->getSampleBufferSize(0)
			<< "," << (std::string)pcmSample->getHighValue()
			<< "," << (std::string)pcmSample->getZeroValue()
			<< "," << (std::string)pcmSample->getLowValue()
			<< std::endl;
	}
	std::cout << std::endl;

	std::cout << "pos";
	auto sampleCountInCycle = pcmSamples[0]->getPcmData()->getSamplesPerCycle();
	const auto commas(std::string(40, ','));
	auto comma1 = commas.substr(0, (waveFormProperties.size() * channels) - 1);
	for(auto& sp : sampleDataTypeProperties) {
		std::cout << "," << sp.name << comma1;
	}
	std::cout << std::endl;
	auto comma2 = commas.substr(0, channels - 1);
	for(auto& sp : sampleDataTypeProperties) {
		for(auto& wp : waveFormProperties) {
			std::cout << "," << wp.name << comma2;
		}
	}
	std::cout << std::endl;
	for(size_t pos = 0; pos < sampleCountInCycle; pos += channels) {
		std::cout << (pos / channels);
		for(auto& pcmSample : pcmSamples) {
			for(WORD ch = 0; ch < channels; ch++) {
				std::cout << "," << (std::string)(*pcmSample)[pos + ch];
			}
		}
		std::cout << std::endl;
	}

	return 0;
}
