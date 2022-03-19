#pragma once

#include "PcmData.h"

#include <string>

/*
 * IPcmSample interface.
 *
 * Utility to access each PCM sample without knowledge of sample type: such as bits/sample(8/16/...), representation(unsigned/signed or integer/float)
 * To create object implementing this interface, call createPcmSample() function with IPcmData object as it's parameter.
 *
 * get()/set() methods use double as sample data type that is enough to hold PCM sample upto 24bit and IEEE float.
 */
class IPcmSample
{
public:
	virtual ~IPcmSample() {}

	virtual IPcmData* getPcmData() const = 0;
	virtual double operator[](size_t index) const = 0;
	virtual std::string getString(size_t index) const = 0;
	virtual double getHighValue() const = 0;
	virtual double getZeroValue() const = 0;
	virtual double getLowValue() const = 0;
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
