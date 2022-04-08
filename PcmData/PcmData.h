#pragma once

#include "framework.h"
#include <memory>
#include <vector>

class IWaveGenerator;

/*
 * IPcmData interface.
 */
class IPcmData
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
	virtual HRESULT copyTo(void* destBuffer, size_t destSize) = 0;

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
	// If duration == 0:
	//		Returns size for 1 cycle samples.
	//		In this case, call after generate(key, ...) method,
	//		because the size depends on the key in addition to the creattion parameters.
	virtual size_t getSampleBufferSize(size_t duration) const = 0;
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

	static const float DefaultDuty;
	static const float DefaultPeakPosition;

	using Factory = IWaveGenerator* (*)(IPcmData::SampleDataType, float);

	struct WaveFormProperty {
		IPcmData::WaveFormType type;
		const char* name;
		Factory factory;
		FactoryParameter parameter;
		float defaultParameter;
	};

	static const std::vector<SampleDataTypeProperty>& getSampleDatatypeProperties();
	static const std::vector<WaveFormProperty>& getWaveFormProperties();

	static const SampleDataTypeProperty& getSampleDataTypeProperty(IPcmData::SampleDataType);
	static const WaveFormProperty& getWaveFormProperty(IPcmData::WaveFormType);
};

// Factory functions.
std::shared_ptr<IPcmData> createPcmData(DWORD samplesPerSec, WORD channels, IWaveGenerator* waveGenerator);
IWaveGenerator* createSquareWaveGenerator(IPcmData::SampleDataType sampleDataType, float duty = PcmDataEnumerator::DefaultDuty);
IWaveGenerator* createSineWaveGenerator(IPcmData::SampleDataType sampleDataType, float notUsed = 0);
IWaveGenerator* createTriangleWaveGenerator(IPcmData::SampleDataType sampleDataType, float peakPosition = PcmDataEnumerator::DefaultPeakPosition);


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
