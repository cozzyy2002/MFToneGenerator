#pragma once

#include "PcmSample.h"
#include "PcmDataImpl.h"

#include <atlbase.h>

namespace
{

// Helper interface to convert sample data using IPcmSample::Value::Handle.
class IValueHelper
{
public:
	using Value = IPcmSample::Value;
	using Handle = IPcmSample::Value::Handle;

	virtual INT32 toInt32(const Handle&) const = 0;
	virtual double toDouble(const Handle&) const = 0;
	virtual std::string toString(const Handle&) const = 0;
};

// Implementation of IValueHelper for sample data of type T.
// 
// IPcmSample::Value::Handle contains:
//	p[0] = Pointer to IValueHelper object
//	p[1] = Pointer to sample data
template<typename T>
class ValueHelper : public IValueHelper
{
public:
	INT32 toInt32(const Handle&) const override;
	double toDouble(const Handle&) const override;
	std::string toString(const Handle& handle) const override;

	// Returns IPcmSample::Value object.
	static Value createValue(const T* sample) {
		static ValueHelper<T> helper;
		Handle handle = { &helper, (void*)sample };
		return Value(handle);
	}

	// Returns sample data of type T.
	T& getSample(const Handle& handle) const { return *(T*)handle.p[1]; }
};

// Returns Pointer of IValueHelper object.
IValueHelper* getHelper(const IValueHelper::Handle& handle) { return (IValueHelper*)handle.p[0]; }

template<typename T>
INT32 ValueHelper<T>::toInt32(const Handle& handle) const
{
	return (INT32)getSample(handle);
}

template<typename T>
double ValueHelper<T>::toDouble(const Handle& handle) const
{
	return (double)getSample(handle);
}

template<typename T>
std::string ValueHelper<T>::toString(const Handle& handle) const
{
	char s[30] = "";
	_itoa_s(*(T*)handle.p[1], s, 10);
	return s;
}

template<>
std::string ValueHelper<float>::toString(const Handle& handle) const
{
	char s[20] = "";
	_gcvt_s(s, *(float*)handle.p[1], 8);
	return s;
}

}


IPcmSample::Value::operator INT32() const
{
	return getHelper(m_handle)->toInt32(m_handle);
}

IPcmSample::Value::operator double() const
{
	return getHelper(m_handle)->toDouble(m_handle);
}

IPcmSample::Value::operator std::string() const
{
	return getHelper(m_handle)->toString(m_handle);
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

	virtual WORD getFormatTag() const override { return PcmData<T>::FormatTag; }

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
	auto pValue = isValid(index) ? &m_buffer[index] : &PcmData<T>::ZeroValue;

	return ValueHelper<T>::createValue(pValue);
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
