// makeWAV.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <mmreg.h>
#include <iostream>
#include <fstream>
#include <memory>

int main(int argc, char* argv[])
{
	auto duty = 0.5f;
	auto peakPosition = 0.5f;
	WORD samplesPerSecond = 44100;
	WORD channels = 1;
	WORD key = 440;
	float level = 1.0f;
	float phaseShift = 0;
	WORD sec = 10;
	IPcmData::SampleDataType type = IPcmData::SampleDataType::IEEE_Float;
	std::string wavFileName;

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
		if(sscanf_s(argv[i], "sec=%d", &iVal) == 1) { sec = iVal; continue; }
		wavFileName = argv[i];
	}
	if(wavFileName.empty()) {
		std::cout << "Usage: makeWAV [duty=Duty] [peak=PeakPosition] [sps=SamplesPerSecond] [ch=Channels] [key=Key] [lvl=Level] [sft=PhaseSift] [sec=Second] WAVFileName\n";
		return 0;
	}

	std::cout << "Generating " << wavFileName
		<< "\n  Duty=" << duty
		<< ", Peak Position=" << peakPosition
		<< ", Samples Per Second=" << samplesPerSecond
		<< ", Channels=" << channels
		<< ", Key=" << key
		<< ", Level=" << level
		<< ", Phase Shift=" << phaseShift
		<< ", Second=" << sec
		<< "\n\n";

	std::ofstream wavFile(wavFileName);

	auto gen = createSineWaveGenerator(type);
	CComPtr<IPcmData> pcmData(createPcmData(samplesPerSecond, channels, gen));
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
