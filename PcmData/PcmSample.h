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
 * Type operators of Value class act depending on audio format type as followings.
 * (Audio format type can be retrieved by IPcmSample::getFormatTag() method.)
 * 		operator		WAVE_FORMAT_PCM					WAVE_FORMAT_IEEE_FLOAT
 *		--------------- ------------------------------- ----------------------
 *		INT32()			Returns as the sample is.		Returns 0 or 1.
 *														(Sample value of this type is 0.0f ~ 1.0f)
 *		double()		Returns as the sample is.		Returns as the sample is.
 *						(Some error may be ovserved)
 *		std::string()	Returns decimal string.			Returns floating-point string.
 */
class IPcmSample
{
public:
	class Value
	{
	public:
		// Opaque object to access sample data.
		struct Handle { void* p[2]; };

		Value(Handle& handle) : m_handle(handle) {}

		operator INT32() const;
		operator double() const;
		operator std::string() const;

	protected:
		Handle m_handle;
	};

	virtual ~IPcmSample() {}

	virtual IPcmData* getPcmData() const = 0;
	virtual Value operator[](size_t index) const = 0;
	virtual Value getHighValue() const = 0;
	virtual Value getZeroValue() const = 0;
	virtual Value getLowValue() const = 0;
	virtual bool isValid(size_t index) const = 0;

	// Returns audio format type used in WAVEFORMAT structure.
	// WAVE_FORMAT_PCM or WAVE_FORMAT_IEEE_FLOAT.
	virtual WORD getFormatTag() const = 0;
};

// Creates IPcmSample object to access internal buffer of IPcmData object taht contains 1 cycle samples.
//   Note:
//		This function should be called after IPcmData::generate() that creates internal buffer.
//		After IPcmData::generate() is called again, re-create IPcmSample object because the method re-creates the internal buffer.
IPcmSample* createPcmSample(IPcmData* pcmData);

// Creates IPcmSample object to access buffer specified as arguments.
//   Note:
//		sampleBuffer should be available in the lifetime of IPcmSample object.
//		bytesInBuffer should be boundary of sample data size. You can use the value returned from IPcmData::getSampleBufferSize(duration).
IPcmSample* createPcmSample(IPcmData::SampleDataType sampleDataType, void* buffer, size_t bytesInBuffer);