#include "PcmSampleImpl.h"

IPcmSample* createPcmSample(std::shared_ptr<IPcmData>& pcmData)
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

IPcmSample* createPcmSample(IPcmData::SampleDataType sampleDataType, void* buffer, size_t size)
{
	switch(sampleDataType) {
	case IPcmData::SampleDataType::PCM_8bits:
		return createPcmSample<UINT8>(buffer, size);
	case IPcmData::SampleDataType::PCM_16bits:
		return createPcmSample<INT16>(buffer, size);
	case IPcmData::SampleDataType::PCM_24bits:
		return createPcmSample<INT24>(buffer, size);
	case IPcmData::SampleDataType::IEEE_Float:
		return createPcmSample<float>(buffer, size);
	default: return nullptr;
	}
}

template<typename T>
const IPcmSample::Value _HighValue = ValueHelper<T>::createValue(&PcmData<T>::HighValue);

template<typename T>
/*static*/ const IPcmSample::Value& IPcmSample::HighValue(_HighValue<T>);

template<typename T>
const IPcmSample::Value _ZeroValue = ValueHelper<T>::createValue(&PcmData<T>::ZeroValue);

template<typename T>
/*static*/ const IPcmSample::Value& IPcmSample::ZeroValue(_ZeroValue<T>);

template<typename T>
const IPcmSample::Value _LowValue = ValueHelper<T>::createValue(&PcmData<T>::LowValue);

template<typename T>
/*static*/ const IPcmSample::Value& IPcmSample::LowValue(_LowValue<T>);

/*static*/ const IPcmSample::Value& IPcmSample::getHighValue(IPcmData::SampleDataType sampleDataType)
{
	switch(sampleDataType) {
	case IPcmData::SampleDataType::PCM_8bits:
		return HighValue<UINT8>;
	case IPcmData::SampleDataType::PCM_16bits:
		return HighValue<INT16>;
	case IPcmData::SampleDataType::PCM_24bits:
		return HighValue<INT24>;
	case IPcmData::SampleDataType::IEEE_Float:
		return HighValue<float>;
	default:
		{
			static Value value;
			return value;
		}
	}
}

/*static*/ const IPcmSample::Value& IPcmSample::getZeroValue(IPcmData::SampleDataType sampleDataType)
{
	switch(sampleDataType) {
	case IPcmData::SampleDataType::PCM_8bits:
		return ZeroValue<UINT8>;
	case IPcmData::SampleDataType::PCM_16bits:
		return ZeroValue<INT16>;
	case IPcmData::SampleDataType::PCM_24bits:
		return ZeroValue<INT24>;
	case IPcmData::SampleDataType::IEEE_Float:
		return ZeroValue<float>;
	default:
		{
			static Value value;
			return value;
		}
	}
}

/*static*/ const IPcmSample::Value& IPcmSample::getLowValue(IPcmData::SampleDataType sampleDataType)
{
	switch(sampleDataType) {
	case IPcmData::SampleDataType::PCM_8bits:
		return LowValue<UINT8>;
	case IPcmData::SampleDataType::PCM_16bits:
		return LowValue<INT16>;
	case IPcmData::SampleDataType::PCM_24bits:
		return LowValue<INT24>;
	case IPcmData::SampleDataType::IEEE_Float:
		return LowValue<float>;
	default:
		{
			static Value value;
			return value;
		}
	}
}
