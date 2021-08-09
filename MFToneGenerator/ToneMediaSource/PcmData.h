#pragma once

class IWaveGenerator;

/*
 * IPcmData interface.
 */
class IPcmData : public IUnknown
{
public:
	virtual ~IPcmData() {}

	enum class SampleDataType {
		_8bits,			// UINT8, WAVE_FORMAT_PCM
		_16bits,		// INT16, WAVE_FORMAT_PCM
		IEEE_Float,		// float, WAVE_FORMAT_IEEE_FLOAT
	};

	static IPcmData* create(WORD samplesPerSec, WORD channels, IWaveGenerator* waveGenerator);
	static IWaveGenerator* createSquareWaveGenerator(SampleDataType sampleDataType, float duty = 0.5f);
	static IWaveGenerator* createSineWaveGenerator(SampleDataType sampleDataType);
	static IWaveGenerator* createTriangleWaveGenerator(SampleDataType sampleDataType, float peakPosition = 0.5f);

	// Generates 1-cycle PCM data
	// Data to be generated depends on IWaveGenerator object passed to the create() static method.
	virtual void generate(float key, float level = 0.2f, float phaseShift = 0) = 0;

	// Copies data generated by generate() method to the buffer.
	// Pass value returned by getSampleBufferSize() method as destSize parameter.
	virtual HRESULT copyTo(BYTE* destBuffer, size_t destSize) = 0;

	virtual WORD getFormatTag() const = 0;
	virtual const char* getWaveForm() const = 0;
	virtual WORD getBlockAlign() const = 0;
	virtual WORD getBitsPerSample() const = 0;
	virtual WORD getSamplesPerSec() const = 0;
	virtual WORD getChannels() const = 0;
	virtual const char* getSampleTypeName() const = 0;
	virtual size_t getSampleCountInCycle() const = 0;		// Available after generate() method is called.
	virtual size_t getSampleBufferSize(size_t duration) const = 0;
};

class IWaveGenerator
{
public:
	virtual ~IWaveGenerator() {}

	virtual IPcmData::SampleDataType getSampleDatatype() const = 0;
};
