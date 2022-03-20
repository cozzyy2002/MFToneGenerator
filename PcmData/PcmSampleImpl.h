#pragma once

#include "PcmSample.h"
#include "PcmDataImpl.h"

#include <atlbase.h>

namespace
{

class IValueHandler
{
public:
	virtual INT32 toInt32(void* p) const = 0;
	virtual double toDouble(void* p) const = 0;
	virtual std::string toString(void* p) const = 0;
};

template<typename T>
class ValueHandler : public IValueHandler
{
public:
	INT32 toInt32(void* p) const override;
	double toDouble(void* p) const override;
	std::string toString(void* p) const override;

	static void setupValueParams(void** pp, T* pSample) {
		static ValueHandler<T> handler;
		pp[0] = &handler;
		pp[1] = pSample;
	}
};

template<typename T>
INT32 ValueHandler<T>::toInt32(void* p) const
{
	return (INT32)(*(T*)p);
}

template<typename T>
double ValueHandler<T>::toDouble(void* p) const
{
	return (double)(*(T*)p);
}

template<typename T>
std::string ValueHandler<T>::toString(void* p) const
{
	char s[20] = "";
	_gcvt_s(s, (T)(*(T*)p), 8);
	return s;
}

template<>
std::string ValueHandler<float>::toString(void* p) const
{
	char s[20] = "";
	_gcvt_s(s, (float)(*(float*)p), 8);
	return s;
}

}


IPcmSample::Value::operator INT32() const
{
	return ((IValueHandler*)m_pp[0])->toInt32(m_pp[1]);
}

IPcmSample::Value::operator double() const
{
	return ((IValueHandler*)m_pp[0])->toDouble(m_pp[1]);
}

IPcmSample::Value::operator std::string() const
{
	return ((IValueHandler*)m_pp[0])->toString(m_pp[1]);
}

template<typename T>
class PcmSampleImpl : public IPcmSample
{
public:
	PcmSampleImpl(IPcmData* pcmData);
	PcmSampleImpl(void* buffer, size_t bytesInBuffer);

	virtual IPcmData* getPcmData() const override { return m_pcmData; }
	virtual Value operator[](size_t index) const override;
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
IPcmSample::Value PcmSampleImpl<T>::operator[](size_t index) const
{
	if(!isValid(index)) return 0;

	void* pp[2];
	ValueHandler<T>::setupValueParams(pp, &m_buffer[index]);
	return Value(pp);
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
