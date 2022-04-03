#pragma once

#include "PcmData.h"

#include <string>
#include <memory>

/*
 * IPcmSample interface.
 *
 * Utility to access each PCM sample without knowledge of sample type: such as bits/sample(8/16/...), representation(unsigned/signed or integer/float)
 * To create object implementing this interface, call createPcmSample() function with IPcmData object as it's parameter.
 *
 * Sample data is retrieved as Value object regardless of original sample data type:
 * Value class provides converting sample data to integer, double and string.
 * 
 * Each operator of Value class acts depending on audio format type as followings.
 * (Audio format type can be retrieved by IPcmSample::getFormatTag() method.)
 * 		operator		WAVE_FORMAT_PCM					WAVE_FORMAT_IEEE_FLOAT
 *		--------------- ------------------------------- ----------------------------------------------
 *		INT32()			Returns as the sample is.		Returns 0 or 1.
 *														(Sample value of this type is 0.0f ~ 1.0f)
 *		double()		Returns as the sample is.		Returns as the sample is.
 *						(Some errors may be observed)
 *		std::string()	Returns decimal string.			Returns floating-point string.
 * 
 *		isFloat()		Returns false.					Returns true;
 *		getInt32()		Returns as the sample is.		Returns (sample * 0x7fffff) as INT32.
 *		setInt32()		Sets as the sample is.			Sets (INT32 value / 0x7fffff) as float.
 * 
 * Where 0x7fffff is positive max value of INT24 type.
 * 
 * CAUTION
 *   Do not write to the Value object copied from const Value or write access violation occurs.
 *   Example:
 *      auto value = IPcmSample::HighValue<INT16>;
 *      value = 0;   <--- write access violation.
 */
class IPcmSample
{
public:
	// The object of this class will be returned by IPcmData
	// and can be used to retrieve sample data as integer, double or string.
	class Value
	{
	public:
		// Opaque object to access sample data.
		struct Handle { void* p[2]; };

		Value();
		Value(Handle& handle);

		operator INT32() const;
		operator double() const;
		operator std::string() const;

		Value& operator =(INT32);
		Value& operator =(double);
		Value& operator =(const Value&);

		bool isFloat() const;
		INT32 getInt32() const;
		void setInt32(INT32);

	protected:
		const Handle m_handle;
	};

	virtual ~IPcmSample() {}

	virtual IPcmData* getPcmData() const = 0;
	virtual Value operator[](size_t index) const = 0;
	virtual bool isValid(size_t index) const = 0;
	virtual size_t getSampleCount() const = 0;

	// Returns audio format type used in WAVEFORMAT structure.
	// WAVE_FORMAT_PCM or WAVE_FORMAT_IEEE_FLOAT.
	virtual WORD getFormatTag() const = 0;

	static const Value& getHighValue(IPcmData::SampleDataType);
	static const Value& getZeroValue(IPcmData::SampleDataType);
	static const Value& getLowValue(IPcmData::SampleDataType);

	template<typename T> static const Value& HighValue;
	template<typename T> static const Value& ZeroValue;
	template<typename T> static const Value& LowValue;
};

// Creates IPcmSample object to access internal buffer of IPcmData object that contains 1 cycle samples.
//   Note:
//		This function should be called after IPcmData::generate() that creates internal buffer.
//		After IPcmData::generate() is called again, re-create IPcmSample object because the method re-creates the internal buffer.
IPcmSample* createPcmSample(std::shared_ptr<IPcmData>& pcmData);

// Creates IPcmSample object to access buffer specified as arguments.
//   Note:
//		sampleBuffer should be available in the lifetime of IPcmSample object.
//		size should be boundary of sample data size.
//		You can use the value returned from IPcmData::getSampleBufferSize(duration),
//		if samples in the buffer were retrieved by IPcmData::copyTo() method.
IPcmSample* createPcmSample(IPcmData::SampleDataType sampleDataType, void* buffer, size_t size);

template<typename T>
IPcmSample* createPcmSample(T* buffer, size_t sampleCount);

template<typename T>
IPcmSample* createPcmSample(void* buffer, size_t size);

template<typename T, size_t sampleCount>
IPcmSample* createPcmSample(T (&buffer)[sampleCount])
{
	return createPcmSample(buffer, sampleCount);
}
