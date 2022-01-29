// makeWAV.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <mmreg.h>
#include <iostream>
#include <fstream>
#include <memory>

struct WaveGeneratorFactory
{
	const char* name;

	using func_t = IWaveGenerator* (*)(IPcmData::SampleDataType sampleDataType);
	func_t func;
};

struct SampleDataType
{
	const char* name;
	IPcmData::SampleDataType type;
};

namespace
{
auto duty = 0.5f;
auto peakPosition = 0.5f;
WORD samplesPerSecond = 44100;
WORD channels = 1;
WORD key = 440;
float level = 1.0f;
float phaseShift = 0;
WORD sec = 10;
WaveGeneratorFactory::func_t waveGeneratorFactoryFunc = nullptr;
IPcmData::SampleDataType sampleDataType = IPcmData::SampleDataType::Unknown;
std::string wavFileName;

const WaveGeneratorFactory factories[] = {
	{"Square", [](IPcmData::SampleDataType sampleDataType)
	{
		std::cout << "Creating SquareWaveGenerator(duty=" << duty << ")\n";
		return createSquareWaveGenerator(sampleDataType, duty);
	}},
	{"Sine", [](IPcmData::SampleDataType sampleDataType)
	{
		std::cout << "Creating SineWaveGenerator\n";
		return createSineWaveGenerator(sampleDataType);
	}},
	{"Triangle", [](IPcmData::SampleDataType sampleDataType)
	{
		std::cout << "Creating TriangleWaveGenerator(peakPosition=" << peakPosition << ")\n";
		return createTriangleWaveGenerator(sampleDataType, peakPosition);
	}},
};

const SampleDataType sampleDataTypes[] = {
	{ "8", IPcmData::SampleDataType::PCM_8bits},
	{ "16", IPcmData::SampleDataType::PCM_16bits},
	{ "24", IPcmData::SampleDataType::PCM_24bits},
	{ "32", IPcmData::SampleDataType::IEEE_Float},
};
}

int main(int argc, char* argv[])
{
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
				for(auto& f : factories) {
					if(_strnicmp(f.name, arg, strlen(arg)) == 0) {
						waveGeneratorFactoryFunc = f.func;
						break;
					}
				}
				if(!waveGeneratorFactoryFunc) {
					std::cerr << "Unknown wave form: " << arg << std::endl;
					argError = true;
				}
				break;
			case 1:
				// Sample data type
				for(auto& s : sampleDataTypes) {
					if(_strnicmp(s.name, arg, strlen(arg)) == 0) {
						sampleDataType = s.type;
						break;
					}
				}
				if(sampleDataType == IPcmData::SampleDataType::Unknown) {
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

	if(argError || !waveGeneratorFactoryFunc || (sampleDataType == IPcmData::SampleDataType::Unknown) || wavFileName.empty()) {
		std::cout << "Usage:"
			" makeWAV [duty=Duty] [peak=PeakPosition] [sps=SamplesPerSecond] [ch=Channels] [key=Key] [lvl=Level] [sft=PhaseSift] [sec=Second]"
			" WaveForm SampleDataType WAVFileName"
			"\n";
		return 1;
	}

	auto gen = waveGeneratorFactoryFunc(sampleDataType);
	CComPtr<IPcmData> pcmData(createPcmData(samplesPerSecond, channels, gen));

	std::cout << "Generating " << pcmData->getWaveForm() << "(" << pcmData->getSampleDataTypeName() << ") to " << wavFileName
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
			char name[4];
			DWORD size;
		} riff;
		struct Wave {
			char name[4];
			struct Fmt {
				char name[4];
				DWORD size;
				PCMWAVEFORMAT format;
			} fmt;
			struct Data {
				char name[4];
				DWORD size;
			} data;
		} wave;
	} header;
	header.riff = { { 'R', 'I', 'F', 'F' }, sizeof(header.wave) + dataSize };
	header.wave = { { 'W', 'A', 'V', 'E' } };
	header.wave.fmt = {
		{ 'f', 'm', 't', ' ' }, sizeof(header.wave.fmt.format),
		{
			pcmData->getFormatTag(), pcmData->getChannels(), pcmData->getSamplesPerSec(),
			(DWORD)pcmData->getSamplesPerSec() * pcmData->getBlockAlign(),
			pcmData->getBlockAlign(), pcmData->getBitsPerSample()
		}
	};
	header.wave.data = { { 'd', 'a', 't', 'a' }, dataSize };
	wavFile.write((const char*)&header, sizeof(header));

	for(size_t totalSec = 0; totalSec < sec; totalSec += duration) {
		if(FAILED(HR_EXPECT_OK(pcmData->copyTo(buffer.get(), bufferSize)))) break;
		wavFile.write((const char*)buffer.get(), bufferSize);
	}
}


/*static*/ HRESULT tsm::Assert::checkHResult(HRESULT hr, LPCTSTR exp, LPCTSTR sourceFile, int line)
{
	return hr;
}
