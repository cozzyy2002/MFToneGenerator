#pragma once

#include "PcmSample.h"
#include "PcmDataImpl.h"

#include <atlbase.h>

template<typename T>
class PcmSampleImpl : public IPcmSample
{
public:
	PcmSampleImpl(IPcmData* pcmData);
	PcmSampleImpl(void* buffer, size_t bytesInBuffer);

	virtual IPcmData* getPcmData() const override { return m_pcmData; }
	virtual double get(size_t index) const override;
	virtual void set(double value, size_t index) override;
	virtual std::string getString(size_t siIndex) const override;
	virtual double getHighValue() const override { return PcmData<T>::HighValue; }
	virtual double getZeroValue() const override { return PcmData<T>::ZeroValue; }
	virtual double getLowValue() const override { return PcmData<T>::LowValue; }
	virtual bool isValid(size_t index) const override;

protected:
	CComPtr<PcmData<T>> m_pcmData;

	T* m_buffer;
	size_t m_samplesInBuffer;
};

template<typename T>
PcmSampleImpl<T>::PcmSampleImpl(IPcmData* pcmData)
	: m_pcmData((PcmData<T>*)pcmData)
	, m_buffer(nullptr), m_samplesInBuffer(0)
{
	if(m_pcmData) {
		m_buffer = m_pcmData->m_cycleData.get();
		m_samplesInBuffer = m_pcmData->getSamplesPerCycle();
	}
}

template<typename T>
PcmSampleImpl<T>::PcmSampleImpl(void* buffer, size_t bytesInBuffer)
	: m_buffer((T*)buffer)
	, m_samplesInBuffer(bytesInBuffer / sizeof(T))
{
}

template<typename T>
double PcmSampleImpl<T>::get(size_t index) const
{
	if(!isValid(index)) return 0;

	return (double)m_buffer[index];
}

template<typename T>
void PcmSampleImpl<T>::set(double value, size_t index)
{
	if(!isValid(index)) return;

	m_buffer[index] = (T)value;
}

template<typename T>
std::string PcmSampleImpl<T>::getString(size_t index) const
{
	if(!isValid(index)) return "?";

	char s[30];
	_itoa_s(m_buffer[index], s, 10);
	return s;
}


template<>
std::string PcmSampleImpl<float>::getString(size_t index) const
{
	if(!isValid(index)) return "?";

	char s[20] = "";
	_gcvt_s(s, m_buffer[index], 8);
	return s;
}

template<typename T>
bool PcmSampleImpl<T>::isValid(size_t index) const
{
	return
		(m_buffer ? true : false) &&
		(index < m_samplesInBuffer);
}
