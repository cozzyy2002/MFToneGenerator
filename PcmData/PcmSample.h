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
 * Value object provides converting sample data to/from integer and to string.
 */
class IPcmSample
{
public:
	class Value
	{
	public:
		Value(void** pp) {
			m_pp[0] = pp[0];
			m_pp[1] = pp[1];
		}

		operator INT32() const;
		operator double() const;
		operator std::string() const;

	protected:
		void* m_pp[2];
	};

	virtual ~IPcmSample() {}

	virtual IPcmData* getPcmData() const = 0;
	virtual Value operator[](size_t index) const = 0;
	virtual Value getHighValue() const = 0;
	virtual Value getZeroValue() const = 0;
	virtual Value getLowValue() const = 0;
	virtual bool isValid(size_t index) const = 0;
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
