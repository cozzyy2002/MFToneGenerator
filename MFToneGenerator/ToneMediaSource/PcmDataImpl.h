#pragma once

#include "PcmData.h"
#include "Utils.h"

#include <math.h>

template<typename T>
class WaveGenerator : public IWaveGenerator
{
public:
	virtual ~WaveGenerator() {}

	virtual IPcmData::SampleDataType getSampleDatatype() const override { return SampleDataType; }

	virtual const char* getWaveForm() const = 0;
	virtual void generate(T* cycleData, size_t sampleCountInCycle, WORD channels, float level) = 0;

protected:
	static const IPcmData::SampleDataType SampleDataType;
};

/*
 * WaveGenerator template abstract class
 * Type parameter T is data type of wave sample data.
 *   Such as T=INT16 for 16 bit per sample per channel.
 *   This case, minimum atomic unit of 2-channel sample is 16 / 8 * 2 byte.
 * 
 * IWaveGenerator::generate() and getFormatTag() methods should be implemented by derived class.
 */
template<typename T>
class PcmData : public IPcmData
{
public:
	PcmData(WORD samplesPerSec, WORD channels, IWaveGenerator* waveGenerator)
		: m_samplesPerSec(samplesPerSec), m_channels(channels)
		, m_waveGenerator((WaveGenerator<T>*)waveGenerator)
		, m_sampleCountInCycle(0), m_currentPosition(0)
		, m_unknownImpl(this) {}

	virtual HRESULT copyTo(BYTE* destBuffer, size_t destSize) override;
	virtual void generate(float key, float level, float phaseShift) override;

	virtual WORD getFormatTag() const override { return FormatTag; }
	virtual const char* getWaveForm() const override { return m_waveGenerator->getWaveForm(); }
	// Returns byte size of the minimum atomic unit of data to be generated.
	virtual WORD getBlockAlign() const override { return m_channels * sizeof(T); }
	virtual WORD getBitsPerSample() const override { return sizeof(T) * 8; }
	virtual WORD getSamplesPerSec() const override { return m_samplesPerSec; }
	virtual WORD getChannels() const override { return m_channels; }
	virtual const char* getSampleTypeName() const { return typeid(T).name(); }
	virtual size_t getSampleCountInCycle() const { return m_sampleCountInCycle; }
	virtual size_t getSampleBufferSize(size_t duration) const { return m_samplesPerSec * getBlockAlign() * duration / 1000; }

	static const WORD FormatTag;
	static const T HighValue;
	static const T ZeroValue;
	static const T LowValue;

protected:
	const WORD m_samplesPerSec;
	const WORD m_channels;
	size_t m_sampleCountInCycle;
	size_t m_currentPosition;
	std::unique_ptr<T[]> m_cycleData;
	std::unique_ptr<WaveGenerator<T>> m_waveGenerator;
	CriticalSection::Object m_cycleDataLock;

#pragma region Implementation of IUnknown
public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override { return m_unknownImpl.QueryInterface(riid, ppvObject); }
	virtual ULONG STDMETHODCALLTYPE AddRef(void) override { return m_unknownImpl.AddRef(); }
	virtual ULONG STDMETHODCALLTYPE Release(void) override { return m_unknownImpl.Release(); }

protected:
	tsm::UnknownImpl<PcmData<T>> m_unknownImpl;
#pragma endregion
};

template<typename T>
HRESULT PcmData<T>::copyTo(BYTE* destBuffer, size_t destSize)
{
	CriticalSection lock(m_cycleDataLock);

	// Assert that data has been generated.
	HR_ASSERT(m_cycleData, E_ILLEGAL_METHOD_CALL);

	for(DWORD destPosition = 0; destPosition < destSize; destPosition += sizeof(T)) {
		*(T*)(&destBuffer[destPosition]) = m_cycleData[m_currentPosition++];
		if(m_sampleCountInCycle < m_currentPosition) { m_currentPosition = 0; }
	}

	return S_OK;
}

template<typename T>
void PcmData<T>::generate(float key, float level, float phaseShift)
{
	// Generate PCM data for first channel using WaveGenerator.
	auto sampleCountInCycle = (size_t)(m_channels * m_samplesPerSec / key);
	auto blockAlign = getBlockAlign();
	std::unique_ptr<T[]> cycleData(new T[sampleCountInCycle]);
	m_waveGenerator->generate(cycleData.get(), sampleCountInCycle, m_channels, level);

	// Copy first channel to another channel shifting phase.
	auto shiftDelta = (size_t)((sampleCountInCycle * phaseShift) * m_channels);
	for(WORD channel = 1; channel < m_channels; channel++) {
		auto shift = shiftDelta * channel;
		for(size_t pos = 0; pos < sampleCountInCycle; pos += m_channels) {
			cycleData[pos + channel] = cycleData[(pos + shift) % sampleCountInCycle];
		}
	}

	// Update member variables in the Critical Section.
	{
		CriticalSection lock(m_cycleDataLock);
		m_cycleData = std::move(cycleData);
		m_sampleCountInCycle = sampleCountInCycle;
		m_currentPosition = 0;
	}
}


/*
 * SquareWaveGenerator class derived from WaveGenerator class.
 * 
 * Override of WaveGenerator::generate() method generates Square wave.
 * This class exposes constructor that has duty parameter.
 */
template<typename T>
class SquareWaveGenerator : public WaveGenerator<T>
{
public:
	SquareWaveGenerator(float duty) : m_duty(duty) {}

	virtual const char* getWaveForm() const override { return "Square Wave"; }
	virtual void generate(T* cycleData, size_t sampleCountInCycle, WORD channels, float level) override;

protected:
	float m_duty;
};

template<typename T>
void SquareWaveGenerator<T>::generate(T* cycleData, size_t sampleCountInCycle, WORD channels, float level)
{
	auto highValue = (T)(PcmData<T>::HighValue * level);
	auto lowValue = (T)(PcmData<T>::LowValue * level);
	size_t highDuration = (size_t)(sampleCountInCycle * m_duty);
	size_t pos = 0;
	for(; pos < highDuration; pos += channels) {
		cycleData[pos] = highValue;
	}
	for(; pos < sampleCountInCycle; pos++) {
		cycleData[pos] = lowValue;
	}
}

/*
 * SineWaveGenerator class derived from WaveGenerator class.
 *
 * Override of WaveGenerator::generate() method generates Sine wave.
 */
template<typename T>
class SineWaveGenerator : public WaveGenerator<T>
{
public:
	virtual const char* getWaveForm() const override { return "Sine Wave"; }
	virtual void generate(T* cycleData, size_t sampleCountInCycle, WORD channels, float level) override;
};

template<typename T>
void SineWaveGenerator<T>::generate(T* cycleData, size_t sampleCountInCycle, WORD channels, float level)
{
	static const float pi = 3.141592f;
	auto highValue = (T)((PcmData<T>::HighValue - PcmData<T>::ZeroValue) * level);

	for(size_t pos = 0; pos < sampleCountInCycle; pos += channels) {
		auto radian = 2 * pi * pos / sampleCountInCycle;
		auto value = (T)((sin(radian) * highValue) + PcmData<T>::ZeroValue);
		cycleData[pos] = value;
	}
}
/*
 * TriangleWaveGenerator class derived from WaveGenerator class.
 *
 * Override of WaveGenerator::generate() method generates Triangle wave.
 * This class exposes constructor that has peakPosition parameter.
 * If ((peakPosition <= 0.0f) || ( 1.0f <= peakPosition)), generate() method generates Sawtooth wave.
 */
template<typename T>
class TriangleWaveGenerator : public WaveGenerator<T>
{
public:
	TriangleWaveGenerator(float peakPosition)
	{
		if(peakPosition < 0) { m_peakPosition = 0; }
		else if(1 < peakPosition) { m_peakPosition = 1; }
		else { m_peakPosition = peakPosition; }
	}

	virtual const char* getWaveForm() const override { return "Triangle Wave"; }
	virtual void generate(T* cycleData, size_t sampleCountInCycle, WORD channels, float level) override;

protected:
	float m_peakPosition;
};

template<typename T>
void TriangleWaveGenerator<T>::generate(T* cycleData, size_t sampleCountInCycle, WORD channels, float level)
{
	float highValue = PcmData<T>::HighValue * level;
	float lowValue = PcmData<T>::LowValue * level;
	float height = highValue - lowValue;
	size_t upDuration = (size_t)(sampleCountInCycle * m_peakPosition);
	size_t pos = 0;
	float value = lowValue;
	if(0 < upDuration) {
		auto delta = height / upDuration;
		for(; pos < upDuration; pos += channels) {
			cycleData[pos] = (T)value;
			value += delta;
		}
		// Revert value to the last.
		value -= delta;
	}

	if(upDuration < sampleCountInCycle) {
		size_t downDuration = sampleCountInCycle - upDuration;
		auto delta = height / downDuration;
		if(value == lowValue) { value = highValue + delta; }
		for(; pos < sampleCountInCycle; pos++) {
			value -= delta;
			cycleData[pos] = (T)value;
		}
	}
}
