#include "PcmSampleImpl.h"
#include "INT24.h"

IPcmSample* createPcmSample(IPcmData* pcmData)
{
	if(!pcmData) return nullptr;
	if(!pcmData->getSamplesPerCycle()) return nullptr;	// pcmData->generate() has not been called.

	switch(pcmData->getSampleDataType()) {
	case IPcmData::SampleDataType::PCM_8bits:
		return new PcmSampleImpl<UINT8>(pcmData);
	case IPcmData::SampleDataType::PCM_16bits:
		return new PcmSampleImpl<INT16>(pcmData);
	case IPcmData::SampleDataType::PCM_24bits:
		return new PcmSampleImpl<INT24>(pcmData);
	case IPcmData::SampleDataType::IEEE_Float:
		return new PcmSampleImpl<float>(pcmData);
	default: return nullptr;
	}
}

IPcmSample* createPcmSample(IPcmData::SampleDataType sampleDataType, void* buffer, size_t bytesInBuffer)
{
	switch(sampleDataType) {
	case IPcmData::SampleDataType::PCM_8bits:
		return createPcmSample<UINT8>(buffer, bytesInBuffer);
	case IPcmData::SampleDataType::PCM_16bits:
		return createPcmSample<INT16>(buffer, bytesInBuffer);
	case IPcmData::SampleDataType::PCM_24bits:
		return createPcmSample<INT24>(buffer, bytesInBuffer);
	case IPcmData::SampleDataType::IEEE_Float:
		return createPcmSample<float>(buffer, bytesInBuffer);
	default: return nullptr;
	}
}
