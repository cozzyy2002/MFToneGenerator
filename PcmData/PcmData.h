#pragma once

#include "framework.h"
#include <Unknwn.h>
#include <vector>
#include <string>

class IWaveGenerator;

/*
 * IPcmData interface.
 */
class IPcmData : public IUnknown
{
public:
	virtual ~IPcmData() {}

	enum class SampleDataType {
		Unknown,
		PCM_8bits,		// UINT8, WAVE_FORMAT_PCM
		PCM_16bits,		// INT16, WAVE_FORMAT_PCM
		PCM_24bits,		// INT24, WAVE_FORMAT_PCM
		IEEE_Float,		// float, WAVE_FORMAT_IEEE_FLOAT
	};

	enum class WaveFormType {
		Unknown,
		SquareWave,
		SineWave,
		TriangleWave,
	};

	// Generates 1-cycle PCM data
	// Data to be generated depends on IWaveGenerator object passed to the createPcmData() function.
	virtual void generate(float key, float level = 0.2f, float phaseShift = 0) = 0;

	// Copies data generated by generate() method to the buffer.
	// Pass value returned by getSampleBufferSize() method as destSize parameter.
	virtual HRESULT copyTo(BYTE* destBuffer, size_t destSize) = 0;

	virtual SampleDataType getSampleDataType() const = 0;
	virtual const char* getSampleDataTypeName() const = 0;
	virtual WORD getFormatTag() const = 0;
	virtual WaveFormType getWaveFormType() const = 0;
	virtual const char* getWaveFormTypeName() const = 0;
	virtual WORD getBlockAlign() const = 0;
	virtual WORD getBitsPerSample() const = 0;
	virtual DWORD getSamplesPerSec() const = 0;
	virtual WORD getChannels() const = 0;
	virtual const char* getSampleTypeName() const = 0;
	virtual size_t getSamplesPerCycle() const = 0;		// Available after generate() method is called.

	// Returns required buffer size in bytes for given duration(mSec).
	// If duration == 0, size for 1 cycle samples is returned.
	virtual size_t getSampleBufferSize(size_t duration) const = 0;
};

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
	virtual double get(size_t index) const = 0;
	virtual void set(double value, size_t index) = 0;
	virtual std::string getString(size_t index) const = 0;
	virtual bool isValid(size_t index) const = 0;
};

class PcmDataEnumerator
{
public:
	struct SampleDataTypeProperty {
		IPcmData::SampleDataType type;
		const char* name;
		WORD formatTag;
		WORD bitsPerSample;
	};

	enum class FactoryParameter {
		None,
		Duty,			// SquareWaveGenerator
		PeakPosition,	// TriangleWaveGenerator
	};

	using Factory = IWaveGenerator* (*)(IPcmData::SampleDataType, float);

	struct WaveFormProperty {
		IPcmData::WaveFormType type;
		const char* name;
		Factory factory;
		FactoryParameter parameter;
	};

	static const std::vector<SampleDataTypeProperty>& getSampleDatatypeProperties();
	static const std::vector<WaveFormProperty>& getWaveFormProperties();
};

// Factory functions.
IPcmData* createPcmData(DWORD samplesPerSec, WORD channels, IWaveGenerator* waveGenerator);
IWaveGenerator* createSquareWaveGenerator(IPcmData::SampleDataType sampleDataType, float duty = 0.5f);
IWaveGenerator* createSineWaveGenerator(IPcmData::SampleDataType sampleDataType);
IWaveGenerator* createTriangleWaveGenerator(IPcmData::SampleDataType sampleDataType, float peakPosition = 0.5f);

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

class DoNotCopy
{
public:
	DoNotCopy() {}
	DoNotCopy(const DoNotCopy&) = delete;
	DoNotCopy& operator=(const DoNotCopy&) = delete;

	virtual ~DoNotCopy() {}
};

class CriticalSection : DoNotCopy
{
public:
	class Object : DoNotCopy
	{
		friend class CriticalSection;
	public:
		Object() { InitializeCriticalSection(&m_criticalSection); }
		~Object() { DeleteCriticalSection(&m_criticalSection); }

	protected:
		operator CRITICAL_SECTION* () { return &m_criticalSection; }
		CRITICAL_SECTION m_criticalSection;
	};

	CriticalSection(Object& object) : m_object(object) { EnterCriticalSection(m_object); }
	~CriticalSection() { LeaveCriticalSection(m_object); }

protected:
	Object& m_object;
};
