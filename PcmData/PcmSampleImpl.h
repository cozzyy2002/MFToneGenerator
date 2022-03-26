#pragma once

#include "PcmSample.h"
#include "PcmDataImpl.h"
#include "INT24.h"

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

	virtual void setInt32(const Handle&, INT32) = 0;
	virtual void setDouble(const Handle&, double) = 0;
	virtual void setValue(const Handle&, const Handle&) = 0;

	virtual bool isFloat() const = 0;
};

// Implementation of IValueHelper for sample data of type T.
// 
// IPcmSample::Value::Handle contains:
//	p[0] = Pointer to IValueHelper object
//	p[1] = Pointer to a sample data
template<typename T>
class ValueHelper : public IValueHelper
{
public:
	INT32 toInt32(const Handle&) const override;
	double toDouble(const Handle&) const override;
	std::string toString(const Handle& handle) const override;

	void setInt32(const Handle&, INT32) override;
	void setDouble(const Handle&, double) override;
	void setValue(const Handle&, const Handle&) override;

	bool isFloat() const override;

	// Returns IPcmSample::Value object.
	static Value createValue(const T* sample) {
		// ValueHelper object for sample type T.
		// This object is shared by Value objects those have same sample type.
		static ValueHelper<T> helper;
		Handle handle = { &helper, (void*)sample };
		return Value(handle);
	}

	// Returns sample data of type T.
	T& getSample(const Handle& handle) const { return *(T*)handle.p[1]; }
};

// ValueHelper class used to create Value object by default constructor.
// Methods of this class:
//   ignore it's parameter.
//   return default value of each type T.
//   set nothing.
class NullValueHelper : public IValueHelper
{
public:
	INT32 toInt32(const Handle&) const override { return INT32(); }
	double toDouble(const Handle&) const override { return double(); }
	std::string toString(const Handle& handle) const override { return ""; }

	void setInt32(const Handle&, INT32) override {}
	void setDouble(const Handle&, double) override {}
	void setValue(const Handle&, const Handle&) override {}

	bool isFloat() const override { return false; }

	static NullValueHelper instance;
};

/*static*/ NullValueHelper NullValueHelper::instance;

// Returns Pointer of IValueHelper object.
inline IValueHelper* getHelper(const IValueHelper::Handle& handle) { return (IValueHelper*)handle.p[0]; }

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
	_itoa_s(getSample(handle), s, 10);
	return s;
}

template<>
std::string ValueHelper<float>::toString(const Handle& handle) const
{
	char s[20] = "";
	_gcvt_s(s, getSample(handle), 8);
	return s;
}

template<typename T>
void ValueHelper<T>::setInt32(const Handle& to, INT32 value)
{
	getSample(to) = (T)value;
}

template<typename T>
void ValueHelper<T>::setDouble(const Handle& to, double value)
{
	getSample(to) = (T)value;
}

template<typename T>
void ValueHelper<T>::setValue(const Handle& to, const Handle& value)
{
	// Assignment is performed by objects that have same sample type.
	if(this == getHelper(value)) {
		getSample(to) = getSample(value);
	}
}

template<typename T>
bool ValueHelper<T>::isFloat() const
{
	return false;
}

template<>
bool ValueHelper<float>::isFloat() const
{
	return true;
}

}

// Default Value constructor using NullValueHelper.
IPcmSample::Value::Value()
	: m_handle{ &NullValueHelper::instance, nullptr }
{
}

IPcmSample::Value::Value(Handle& handle)
	: m_handle(handle)
{
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

IPcmSample::Value& IPcmSample::Value::operator =(INT32 value)
{
	getHelper(m_handle)->setInt32(m_handle, value);
	return *this;
}

IPcmSample::Value& IPcmSample::Value::operator =(double value)
{
	getHelper(m_handle)->setDouble(m_handle, value);
	return *this;
}

IPcmSample::Value& IPcmSample::Value::operator =(const Value& value)
{
	getHelper(m_handle)->setValue(m_handle, value.m_handle);
	return *this;
}


bool IPcmSample::Value::isFloat() const
{
	return getHelper(m_handle)->isFloat();
}

INT32 IPcmSample::Value::getInt32() const
{
	if(isFloat()) {
		return (INT32)(this->operator double() * INT24::MaxValue);
	} else {
		return this->operator INT32();
	}
}

void IPcmSample::Value::setInt32(INT32 value)
{
	if(isFloat()) {
		this->operator=((double)value / INT24::MaxValue);
	} else {
		this->operator=(value);
	}
}

template<typename T>
class PcmSampleImpl : public IPcmSample
{
public:
	PcmSampleImpl(IPcmData* pcmData);
	PcmSampleImpl(T* buffer, size_t sampleCount);

	virtual IPcmData* getPcmData() const override { return m_pcmData; }
	virtual Value operator[](size_t index) const override;
	virtual bool isValid(size_t index) const override;
	virtual size_t getSampleCount() const override;

	virtual WORD getFormatTag() const override { return PcmData<T>::FormatTag; }

protected:
	CComPtr<PcmData<T>> m_pcmData;

	T* m_buffer;
	size_t m_sampleCount;
};

template<typename T>
PcmSampleImpl<T>::PcmSampleImpl(IPcmData* pcmData)
	: m_pcmData((PcmData<T>*)pcmData)
	, m_buffer(nullptr), m_sampleCount(0)
{
	if(m_pcmData) {
		m_buffer = m_pcmData->m_cycleData.get();
		m_sampleCount = m_pcmData->getSamplesPerCycle();
	}
}

template<typename T>
PcmSampleImpl<T>::PcmSampleImpl(T* buffer, size_t sampleCount)
	: m_buffer(buffer)
	, m_sampleCount(sampleCount)
{
}

template<typename T>
IPcmSample::Value PcmSampleImpl<T>::operator[](size_t index) const
{
	return isValid(index) ? 
		ValueHelper<T>::createValue(&m_buffer[index]) :
		Value();
}

template<typename T>
bool PcmSampleImpl<T>::isValid(size_t index) const
{
	return (index < m_sampleCount);
}

template<typename T>
size_t PcmSampleImpl<T>::getSampleCount() const
{
	return m_sampleCount;
}

template<typename T>
IPcmSample* createPcmSample(T* buffer, size_t sampleCount)
{
	if(!buffer) return nullptr;
	if(!sampleCount) return nullptr;

	return new PcmSampleImpl<T>(buffer, sampleCount);
}

template<typename T>
IPcmSample* createPcmSample(void* buffer, size_t size)
{
	// Check if buffer size is sample data size boundary.
	if((size % sizeof(T)) != 0) return nullptr;

	return createPcmSample((T*)buffer, size / sizeof(T));
}
