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
	default:
		return nullptr;
	}
}

IPcmSample* createPcmSample(IPcmData::SampleDataType sampleDataType, void* buffer, size_t bytesInBuffer)
{
	if(!buffer) return nullptr;
	if(!bytesInBuffer) return nullptr;

	std::unique_ptr<IPcmSample> pcmSample;
	size_t bytesPerSample = 0;
	switch(sampleDataType) {
	case IPcmData::SampleDataType::PCM_8bits:
		pcmSample.reset(new PcmSampleImpl<UINT8>(buffer, bytesInBuffer));
		bytesPerSample = sizeof(UINT8);
		break;
	case IPcmData::SampleDataType::PCM_16bits:
		pcmSample.reset(new PcmSampleImpl<INT16>(buffer, bytesInBuffer));
		bytesPerSample = sizeof(INT16);
		break;
	case IPcmData::SampleDataType::PCM_24bits:
		pcmSample.reset(new PcmSampleImpl<INT24>(buffer, bytesInBuffer));
		bytesPerSample = sizeof(INT24);
		break;
	case IPcmData::SampleDataType::IEEE_Float:
		pcmSample.reset(new PcmSampleImpl<float>(buffer, bytesInBuffer));
		bytesPerSample = sizeof(float);
		break;
	default:
		return nullptr;
	}

	// Check if buffer size is sample data size boundary.
	if((bytesInBuffer % bytesPerSample) != 0) return nullptr;

	return pcmSample.release();
}

template<>
IPcmSample* createPcmSample<UINT8>(void* buffer, size_t bytesInBuffer)
{
	return createPcmSample(IPcmData::SampleDataType::PCM_8bits, buffer, bytesInBuffer);
}

template<>
IPcmSample* createPcmSample<INT16>(void* buffer, size_t bytesInBuffer)
{
	return createPcmSample(IPcmData::SampleDataType::PCM_16bits, buffer, bytesInBuffer);
}

template<>
IPcmSample* createPcmSample<INT24>(void* buffer, size_t bytesInBuffer)
{
	return createPcmSample(IPcmData::SampleDataType::PCM_24bits, buffer, bytesInBuffer);
}

template<>
IPcmSample* createPcmSample<float>(void* buffer, size_t bytesInBuffer)
{
	return createPcmSample(IPcmData::SampleDataType::IEEE_Float, buffer, bytesInBuffer);
}
