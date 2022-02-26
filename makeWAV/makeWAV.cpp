// makeWAV.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <mmreg.h>
#include <iostream>
#include <fstream>
#include <memory>

int main(int argc, char* argv[])
{
	const auto sampleDataTypeProperties(PcmDataEnumerator::getSampleDatatypeProperties());
	const auto waveFormProperties(PcmDataEnumerator::getWaveFormProperties());

	auto duty = 0.5f;
	auto peakPosition = 0.5f;
	WORD samplesPerSecond = 44100;
	WORD channels = 1;
	WORD key = 440;
	float level = 1.0f;
	float phaseShift = 0;
	WORD sec = 1;
	const PcmDataEnumerator::SampleDataTypeProperty* sampleDataTypeProperty = nullptr;
	const PcmDataEnumerator::WaveFormProperty* waveFormProperty = nullptr;
	std::string wavFileName;

	float fVal;
	int iVal;
	int argIndex = 0;
	bool argError = false;
	for(int i = 1; i < argc; i++) {
		auto arg = argv[i];
		if(strchr(arg, '=')) {
			if(sscanf_s(arg, "duty=%f", &fVal) == 1) { duty = fVal; }
			else if(sscanf_s(arg, "peak=%f", &fVal) == 1) { peakPosition = fVal; }
			else if(sscanf_s(arg, "sps=%d", &iVal) == 1) { samplesPerSecond = iVal; }
			else if(sscanf_s(arg, "ch=%d", &iVal) == 1) { channels = iVal; }
			else if(sscanf_s(arg, "key=%d", &iVal) == 1) { key = iVal; }
			else if(sscanf_s(arg, "lvl=%f", &fVal) == 1) { level = fVal; }
			else if(sscanf_s(arg, "sft=%f", &fVal) == 1) { phaseShift = fVal; }
			else if(sscanf_s(arg, "sec=%d", &iVal) == 1) { sec = iVal; }
			else {
				std::cerr << "Unknown argument: " << arg << std::endl;
				argError = true;
			}
		} else {
			switch(argIndex++) {
			case 0:
				// Wave form
				for(auto& wp : waveFormProperties) {
					if(_strnicmp(wp.name, arg, strlen(arg)) == 0) {
						waveFormProperty = &wp;
						break;
					}
				}
				if(!waveFormProperty) {
					std::cerr << "Unknown wave form: " << arg << std::endl;
					argError = true;
				}
				break;
			case 1:
				// Sample data type
				{
					WORD bitsPerSample = atoi(arg);
					for(auto& sp : sampleDataTypeProperties) {
						if(bitsPerSample == sp.bitsPerSample) {
							sampleDataTypeProperty = &sp;
							break;
						}
					}
				}
				if(!sampleDataTypeProperty) {
					std::cerr << "Unknown sample data type: " << arg << std::endl;
					argError = true;
				}
				break;
			case 2:
				// File name to be genarated.
				wavFileName = arg;
				break;
			default:
				std::cerr << "Too many arguments: " << arg << std::endl;
				argError = true;
				break;
			}
		}
	}

	if(argError || !waveFormProperty || !sampleDataTypeProperty || wavFileName.empty()) {
		std::cerr << "Usage:"
			" makeWAV [duty=Duty] [peak=PeakPosition] [sps=SamplesPerSecond] [ch=Channels] [key=Key] [lvl=Level] [sft=PhaseSift] [sec=Second]"
			" WaveForm SampleDataType WAVFileName";
		std::cerr << "\n    waveForm:";
		for(auto& wp : waveFormProperties) { std::cerr << " '" << wp.name << "'"; };
		std::cerr << std::endl;
		std::cerr << "\n    sampleDataType:";
		for(auto& sp : sampleDataTypeProperties) { std::cerr << " " << sp.bitsPerSample; };
		std::cerr << std::endl;
		return 1;
	}

	float param = 0;
	switch(waveFormProperty->parameter) {
	case PcmDataEnumerator::FactoryParameter::Duty:
		param = duty;
		break;
	case PcmDataEnumerator::FactoryParameter::PeakPosition:
		param = peakPosition;
		break;
	}

	std::cout << "Creating WaveGenerator for " << waveFormProperty->name << "," << sampleDataTypeProperty->bitsPerSample << " bits per sample, Parameter=" << param << std::endl;
	auto gen = waveFormProperty->factory(sampleDataTypeProperty->type, param);
	CComPtr<IPcmData> pcmData(createPcmData(samplesPerSecond, channels, gen));

	std::cout << "Generating " << pcmData->getWaveFormTypeName() << "(" << pcmData->getSampleDataTypeName() << ") to " << wavFileName
		<< "\nSamples Per Second=" << samplesPerSecond
		<< ", Channels=" << channels
		<< ", Key=" << key
		<< ", Level=" << level
		<< ", Phase Shift=" << phaseShift
		<< ", Second=" << sec
		<< "\n\n";

	std::ofstream wavFile(wavFileName);

	pcmData->generate(key, level, phaseShift);
	const size_t duration = 1;
	auto bufferSize = pcmData->getSampleBufferSize(duration * 1000);
	auto buffer = std::make_unique<BYTE[]>(bufferSize);

	/* RIFF waveform audio format
	*  https://ja.wikipedia.org/wiki/WAV
	*
	*   +00 `RIFF`
	*   +04 Size of RIFF chunk that includes following data.
	*   +08 `WAVE` : FourCC of WAV file.
	*   +0c `fmt ` : fmt chunk = WAVEFORMATEX, PCMWAVEFORMAT, WAVEFORMATEXTENSIBLE, ...
	*   +10 Size of fmt chunk. = 0x10(PCMWAVEFORMAT)
	*   +14 wFormatTag
	*   +16 nChannels
	*   +18 nSamplesPerSec
	*   +1c nAvgBytesPerSec
	*   +20 nBlockAlign
	*   +22 wBitsPerSample
	*   +24 `data` : data chunk
	*   +28 Size of data chunk.
	*   +2c PCM data. L-ch -> R-ch
	*/
	DWORD dataSize = bufferSize * sec / duration;
	struct Header {
		struct Riff {
			DWORD chankTag;
			DWORD size;
		} riff;
		struct Wave {
			DWORD chankTag;
			struct Fmt {
				DWORD chankTag;
				DWORD size;
				PCMWAVEFORMAT format;
			} fmt;
			struct Data {
				DWORD chankTag;
				DWORD size;
			} data;
		} wave;
	} header = {
		{ *(DWORD*)"RIFF", sizeof(Header::Wave) + dataSize },
		{ *(DWORD*)"WAVE" , {
			*(DWORD*)"fmt ", sizeof(Header::Wave::Fmt::format), {
					pcmData->getFormatTag(), pcmData->getChannels(), pcmData->getSamplesPerSec(),
					(DWORD)pcmData->getSamplesPerSec() * pcmData->getBlockAlign(),
					pcmData->getBlockAlign(), pcmData->getBitsPerSample()
				},
			},
			{ *(DWORD*)"data", dataSize }
		}
	};

	wavFile.write((const char*)&header, sizeof(header));

	for(size_t totalSec = 0; totalSec < sec; totalSec += duration) {
		if(FAILED(HR_EXPECT_OK(pcmData->copyTo(buffer.get(), bufferSize)))) break;
		wavFile.write((const char*)buffer.get(), bufferSize);
	}
}
