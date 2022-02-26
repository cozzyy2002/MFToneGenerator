#pragma once

#include "PcmData.h"
#include <StateMachine/stdafx.h>
#include <StateMachine/Unknown.h>
#include <StateMachine/Assert.h>

#include <memory>
#include <math.h>
#include <Windows.h>
#include <mmreg.h>

class IWaveGenerator
{
public:
	virtual ~IWaveGenerator() {}

	virtual IPcmData::SampleDataType getSampleDataType() const = 0;
	virtual const char* getSampleDataTypeName() const = 0;
	virtual IPcmData::WaveFormType getWaveFormType() const = 0;
	virtual const char* getWaveFormTypeName() const = 0;

	static const char* SquareWaveFormTypeName;
	static const char* SineWaveFormTypeName;
	static const char* TriangleWaveFormTypeName;
};

template<typename T>
class WaveGenerator : public IWaveGenerator
{
public:
	virtual ~WaveGenerator() {}

	virtual IPcmData::SampleDataType getSampleDataType() const override { return SampleDataType; }
	virtual const char* getSampleDataTypeName() const override { return SampleDataTypeName; }
	virtual IPcmData::WaveFormType getWaveFormType() const override = 0;
	virtual const char* getWaveFormTypeName() const override = 0;

	virtual void generate(T* cycleData, size_t samplesPerCycle, WORD channels, float level) = 0;

	static const IPcmData::SampleDataType SampleDataType;
	static const char* SampleDataTypeName;

protected:
	void adjustLevel(float level, T* pHighValue = nullptr, T* pLowValue = nullptr, T* pZeroValue = nullptr) const;
};

/*
 * WaveGenerator template class
 * Type parameter T is data type of wave sample data.
 *   Such as T=INT16 for 16 bit per sample per channel.
 *   This case, minimum atomic unit of 2-channel sample is 16 / 8 * 2 byte.
 */
template<typename T>
class PcmData : public IPcmData
{
public:
	PcmData(WORD samplesPerSec, WORD channels, IWaveGenerator* waveGenerator)
		: m_samplesPerSec(samplesPerSec), m_channels(channels)
		, m_waveGenerator((WaveGenerator<T>*)waveGenerator)
		, m_samplesPerCycle(0), m_currentPosition(0)
		, m_unknownImpl(this) {}

	virtual HRESULT copyTo(BYTE* destBuffer, size_t destSize) override;
	virtual void generate(float key, float level, float phaseShift) override;

	virtual SampleDataType getSampleDataType() const override { return m_waveGenerator->getSampleDataType(); }
	virtual const char* getSampleDataTypeName() const override { return m_waveGenerator->getSampleDataTypeName(); }

	virtual WORD getFormatTag() const override { return FormatTag; }
	virtual WaveFormType getWaveFormType() const { return m_waveGenerator->getWaveFormType(); }
	virtual const char* getWaveFormTypeName() const override { return m_waveGenerator->getWaveFormTypeName(); }
	// Returns byte size of the minimum atomic unit of data to be generated.
	virtual WORD getBlockAlign() const override { return m_channels * sizeof(T); }
	virtual WORD getBitsPerSample() const override { return sizeof(T) * 8; }
	virtual WORD getSamplesPerSec() const override { return m_samplesPerSec; }
	virtual WORD getChannels() const override { return m_channels; }
	virtual const char* getSampleTypeName() const { return typeid(T).name(); }
	virtual size_t getSamplesPerCycle() const { return m_samplesPerCycle; }
	virtual size_t getSampleBufferSize(size_t duration) const { return m_samplesPerSec * getBlockAlign() * duration / 1000; }

	static const WORD FormatTag;
	static const T HighValue;
	static const T ZeroValue;
	static const T LowValue;

protected:
	const WORD m_samplesPerSec;
	const WORD m_channels;
	size_t m_samplesPerCycle;
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

	HR_ASSERT(destBuffer, E_POINTER);
	HR_ASSERT((destSize % sizeof(T)) == 0, E_BOUNDS);

	for(DWORD destPosition = 0; destPosition < destSize; destPosition += sizeof(T)) {
		*(T*)(&destBuffer[destPosition]) = m_cycleData[m_currentPosition++];
		if(m_samplesPerCycle <= m_currentPosition) { m_currentPosition = 0; }
	}

	return S_OK;
}

template<typename T>
void PcmData<T>::generate(float key, float level, float phaseShift)
{
	// Generate PCM data for first channel using WaveGenerator.
	auto samplesPerCycle = (size_t)(m_channels * m_samplesPerSec / key);
	std::unique_ptr<T[]> cycleData(new T[samplesPerCycle]);
	m_waveGenerator->generate(cycleData.get(), samplesPerCycle, m_channels, level);

	// Copy first channel to another channel shifting phase.
	auto shiftDelta = (size_t)(samplesPerCycle * phaseShift) / m_channels * m_channels;
	auto shift = shiftDelta;
	for(WORD channel = 1; channel < m_channels; channel++) {
		for(size_t pos = 0; pos < samplesPerCycle; pos += m_channels) {
			cycleData[pos + channel] = cycleData[(pos + samplesPerCycle - shift) % samplesPerCycle];
		}
		shift += shiftDelta;
	}

	// Update member variables in the Critical Section.
	{
		CriticalSection lock(m_cycleDataLock);
		m_cycleData = std::move(cycleData);
		m_samplesPerCycle = samplesPerCycle;
		m_currentPosition = 0;
	}
}


template<typename T>
void WaveGenerator<T>::adjustLevel(float level, T* pHighValue, T* pLowValue, T* pZeroValue) const
{
	if(pHighValue) {
		*pHighValue = (T)((PcmData<T>::HighValue - PcmData<T>::ZeroValue) * level) + PcmData<T>::ZeroValue;
	}
	if(pLowValue) {
		*pLowValue = PcmData<T>::ZeroValue - (T)((PcmData<T>::ZeroValue - PcmData<T>::LowValue) * level);
	}
	if(pZeroValue) {
		*pZeroValue = PcmData<T>::ZeroValue;
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

	virtual IPcmData::WaveFormType getWaveFormType() const override { return IPcmData::WaveFormType::SquareWave; }
	virtual const char* getWaveFormTypeName() const override { return IWaveGenerator::SquareWaveFormTypeName; }
	virtual void generate(T* cycleData, size_t samplesPerCycle, WORD channels, float level) override;

protected:
	float m_duty;
};

template<typename T>
void SquareWaveGenerator<T>::generate(T* cycleData, size_t samplesPerCycle, WORD channels, float level)
{
	T highValue, lowValue;
	WaveGenerator<T>::adjustLevel(level, &highValue, &lowValue);
	size_t highDuration = (size_t)(samplesPerCycle * m_duty);
	size_t pos = 0;
	for(; pos < highDuration; pos += channels) {
		cycleData[pos] = highValue;
	}
	for(; pos < samplesPerCycle; pos++) {
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
	virtual IPcmData::WaveFormType getWaveFormType() const override { return IPcmData::WaveFormType::SineWave; }
	virtual const char* getWaveFormTypeName() const override { return IWaveGenerator::SineWaveFormTypeName; }
	virtual void generate(T* cycleData, size_t samplesPerCycle, WORD channels, float level) override;
};

template<typename T>
void SineWaveGenerator<T>::generate(T* cycleData, size_t samplesPerCycle, WORD channels, float level)
{
	static const float pi = 3.141592f;
	T highValue, zeroValue;
	WaveGenerator<T>::adjustLevel(level, &highValue, nullptr, &zeroValue);
	auto height = highValue - zeroValue;

	for(size_t pos = 0; pos < samplesPerCycle; pos += channels) {
		auto radian = 2 * pi * pos / samplesPerCycle;
		cycleData[pos] = (T)((sin(radian) * height) + zeroValue);
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

	virtual IPcmData::WaveFormType getWaveFormType() const override { return IPcmData::WaveFormType::TriangleWave; }
	virtual const char* getWaveFormTypeName() const override { return IWaveGenerator::TriangleWaveFormTypeName; }
	virtual void generate(T* cycleData, size_t samplesPerCycle, WORD channels, float level) override;

protected:
	float m_peakPosition;
};

template<typename T>
void TriangleWaveGenerator<T>::generate(T* cycleData, size_t samplesPerCycle, WORD channels, float level)
{
	T highValue, lowValue, zeroValue;
	WaveGenerator<T>::adjustLevel(level, &highValue, &lowValue, &zeroValue);
	float height = (float)(highValue - lowValue);
	float upDelta, downDelta;
	upDelta = downDelta = height * channels / samplesPerCycle;
	bool up;
	float value;
	if(m_peakPosition == 0.0f) {
		upDelta = height * channels;
		up = false;
		value = (float)highValue;
	} else if(m_peakPosition == 1.0f) {
		downDelta = height * channels;
		up = true;
		value = (float)lowValue;
	} else {
		upDelta /= m_peakPosition;
		downDelta /= (1.0f - m_peakPosition);
		up = true;
		value = (float)zeroValue;
	}

	if((lowValue + upDelta) > highValue) { upDelta = height; }
	if((highValue - downDelta) < lowValue) { downDelta = height; }

	for(size_t pos = 0; pos < samplesPerCycle; pos += channels) {
		cycleData[pos] = (T)value;
		if(up) {
			value += upDelta;
			if(highValue < value) {
				value = (float)highValue;
				up = false;
			}
		}
		else {
			value -= downDelta;
			if(value < lowValue) {
				value = (float)lowValue;
				up = true;
			}
		}
	}
}
