#pragma once

#include "PcmSample.h"
#include "PcmDataImpl.h"

#include <atlbase.h>

namespace
{

class IValueHelper
{
public:
	virtual INT32 toInt32(void* p) const = 0;
	virtual double toDouble(void* p) const = 0;
	virtual std::string toString(void* p) const = 0;
};

template<typename T>
class ValueHelper : public IValueHelper
{
public:
	INT32 toInt32(void* p) const override;
	double toDouble(void* p) const override;
	std::string toString(void* p) const override;

	static IPcmSample::Value createValue(const T* sample) {
		static ValueHelper<T> helper;
		void* pp[2] = { &helper, (void*)sample };
		return IPcmSample::Value(pp);
	}
};

template<typename T>
INT32 ValueHelper<T>::toInt32(void* p) const
{
	return (INT32)(*(T*)p);
}

template<typename T>
double ValueHelper<T>::toDouble(void* p) const
{
	return (double)(*(T*)p);
}

template<typename T>
std::string ValueHelper<T>::toString(void* p) const
{
	char s[20] = "";
	_gcvt_s(s, (T)(*(T*)p), 8);
	return s;
}

template<>
std::string ValueHelper<float>::toString(void* p) const
{
	char s[20] = "";
	_gcvt_s(s, (float)(*(float*)p), 8);
	return s;
}

}


IPcmSample::Value::operator INT32() const
{
	return ((IValueHelper*)m_pp[0])->toInt32(m_pp[1]);
}

IPcmSample::Value::operator double() const
{
	return ((IValueHelper*)m_pp[0])->toDouble(m_pp[1]);
}

IPcmSample::Value::operator std::string() const
{
	return ((IValueHelper*)m_pp[0])->toString(m_pp[1]);
}

template<typename T>
class PcmSampleImpl : public IPcmSample
{
public:
	PcmSampleImpl(IPcmData* pcmData);
	PcmSampleImpl(void* buffer, size_t bytesInBuffer);

	virtual IPcmData* getPcmData() const override { return m_pcmData; }
	virtual Value operator[](size_t index) const override;
	virtual Value getHighValue() const override;
	virtual Value getZeroValue() const override;
	virtual Value getLowValue() const override;
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

	return ValueHelper<T>::createValue(&m_buffer[index]);
}

template<typename T>
IPcmSample::Value PcmSampleImpl<T>::getHighValue() const
{
	return ValueHelper<T>::createValue(&PcmData<T>::HighValue);
}

template<typename T>
IPcmSample::Value PcmSampleImpl<T>::getZeroValue() const
{
	return ValueHelper<T>::createValue(&PcmData<T>::ZeroValue);
}

template<typename T>
IPcmSample::Value PcmSampleImpl<T>::getLowValue() const
{
	return ValueHelper<T>::createValue(&PcmData<T>::LowValue);
}

template<typename T>
bool PcmSampleImpl<T>::isValid(size_t index) const
{
	return
		(m_buffer ? true : false) &&
		(index < m_samplesInBuffer);
}
