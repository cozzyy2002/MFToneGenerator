#include "PcmSampleImpl.h"
#include "INT24.h"

IPcmSample* createPcmSample(IPcmData* pcmData)
{
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
	switch(sampleDataType) {
	case IPcmData::SampleDataType::PCM_8bits:
		return new PcmSampleImpl<UINT8>(buffer, bytesInBuffer);
	case IPcmData::SampleDataType::PCM_16bits:
		return new PcmSampleImpl<INT16>(buffer, bytesInBuffer);
	case IPcmData::SampleDataType::PCM_24bits:
		return new PcmSampleImpl<INT24>(buffer, bytesInBuffer);
	case IPcmData::SampleDataType::IEEE_Float:
		return new PcmSampleImpl<float>(buffer, bytesInBuffer);
	default:
		return nullptr;
	}
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
