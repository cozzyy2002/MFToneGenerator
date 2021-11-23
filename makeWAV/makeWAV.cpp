// makeWAV.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

int main(int argc, char* argv[])
{
	auto duty = 0.5f;
	auto peakPosition = 0.5f;
	WORD samplesPerSecond = 44100;
	WORD channels = 1;
	WORD key = 440;
	float level = 1.0f;
	float phaseShift = 0;
	float sec = 10;
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
		if(sscanf_s(argv[i], "sec=%f", &fVal) == 1) { sec = fVal; continue; }
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
}
