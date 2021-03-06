#pragma once

#include "PcmData.h"
#include <StateMachine/stdafx.h>
#include <StateMachine/Unknown.h>
#include <StateMachine/Assert.h>

#include <memory>
#include <math.h>
#include <Windows.h>
#include <mmreg.h>

template<typename T>
class PcmSampleImpl;

namespace
{
float limit(float value, float max = 1.0f, float min = 0)
{
	if(value < min) return min;
	if(max < value) return max;
	return value;
}
}

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
	template<typename TT>
	friend class PcmSampleImpl;

public:
	PcmData(DWORD samplesPerSec, WORD channels, IWaveGenerator* waveGenerator)
		: m_samplesPerSec(samplesPerSec), m_channels(channels)
		, m_waveGenerator((WaveGenerator<T>*)waveGenerator)
		, m_samplesPerCycle(0), m_currentPosition(0) {}

	virtual HRESULT copyTo(void* destBuffer, size_t destSize) override;
	virtual void generate(float key, float level, float phaseShift) override;

	virtual SampleDataType getSampleDataType() const override { return m_waveGenerator->getSampleDataType(); }
	virtual const char* getSampleDataTypeName() const override { return m_waveGenerator->getSampleDataTypeName(); }

	virtual WORD getFormatTag() const override { return FormatTag; }
	virtual WaveFormType getWaveFormType() const { return m_waveGenerator->getWaveFormType(); }
	virtual const char* getWaveFormTypeName() const override { return m_waveGenerator->getWaveFormTypeName(); }
	// Returns byte size of the minimum atomic unit of data to be generated.
	virtual WORD getBlockAlign() const override { return m_channels * sizeof(T); }
	virtual WORD getBitsPerSample() const override { return sizeof(T) * 8; }
	virtual DWORD getSamplesPerSec() const override { return m_samplesPerSec; }
	virtual WORD getChannels() const override { return m_channels; }
	virtual const char* getSampleTypeName() const { return typeid(T).name(); }
	virtual size_t getSamplesPerCycle() const { return m_samplesPerCycle; }
	virtual size_t getSampleBufferSize(size_t duration) const;

	static const WORD FormatTag;
	static const T HighValue;
	static const T ZeroValue;
	static const T LowValue;

protected:
	const DWORD m_samplesPerSec;
	const WORD m_channels;
	size_t m_samplesPerCycle;
	size_t m_currentPosition;
	std::unique_ptr<T[]> m_cycleData;
	std::unique_ptr<WaveGenerator<T>> m_waveGenerator;
	CriticalSection::Object m_cycleDataLock;

	// Returns number rounded up to the nearest multiple of significance value.
	size_t ceiling(size_t number, size_t significance) const;
};

template<typename T>
HRESULT PcmData<T>::copyTo(void* destBuffer, size_t destSize)
{
	CriticalSection lock(m_cycleDataLock);

	// Assert that data has been generated.
	HR_ASSERT(m_cycleData, E_ILLEGAL_METHOD_CALL);

	HR_ASSERT(destBuffer, E_POINTER);
	HR_ASSERT(0 < destSize, ERROR_INCORRECT_SIZE);
	HR_ASSERT((destSize % getBlockAlign()) == 0, E_BOUNDS);

	for(DWORD destPosition = 0; destPosition < destSize; destPosition += sizeof(T)) {
		*(T*)&((BYTE*)destBuffer)[destPosition] = m_cycleData[m_currentPosition++];
		if(m_samplesPerCycle <= m_currentPosition) { m_currentPosition = 0; }
	}

	return S_OK;
}

template<typename T>
void PcmData<T>::generate(float key, float level, float phaseShift)
{
	// Generate PCM data for first channel using WaveGenerator.
	auto samplesPerCycle = ceiling((size_t)(m_samplesPerSec * m_channels / key), m_channels);
	std::unique_ptr<T[]> cycleData(new T[samplesPerCycle]);
	m_waveGenerator->generate(cycleData.get(), samplesPerCycle, m_channels, limit(level));

	if(1 < m_channels) {
		// Copy first channel to another channel shifting phase.
		auto shiftDelta = ceiling(samplesPerCycle - (size_t)(samplesPerCycle * limit(phaseShift)), m_channels);
		size_t shift = 0;
		for(WORD channel = 1; channel < m_channels; channel++) {
			shift += shiftDelta;
			auto posSrc = shift;
			for(size_t pos = 0; pos < samplesPerCycle; pos += m_channels) {
				cycleData[pos + channel] = cycleData[posSrc % samplesPerCycle];
				posSrc += m_channels;
			}
		}
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
size_t PcmData<T>::getSampleBufferSize(size_t duration) const
{
	if(0 < duration) {
		auto ba = getBlockAlign();
		return ceiling(m_samplesPerSec * ba * duration / 1000, ba);
	} else {
		return m_samplesPerCycle * sizeof(T);
	}
}

template<typename T>
size_t PcmData<T>::ceiling(size_t number, size_t significance) const
{
	auto remain = number % significance;
	if(remain) {
		number += (significance - remain);
	}
	return number;
}


template<typename T>
void WaveGenerator<T>::adjustLevel(float level, T* pHighValue, T* pLowValue, T* pZeroValue) const
{
	if(pHighValue) {
		*pHighValue = (T)((PcmData<T>::HighValue - PcmData<T>::ZeroValue) * (double)level) + PcmData<T>::ZeroValue;
	}
	if(pLowValue) {
		*pLowValue = PcmData<T>::ZeroValue - (T)((PcmData<T>::ZeroValue - PcmData<T>::LowValue) * (double)level);
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
	SquareWaveGenerator(float duty) : m_duty(limit(duty, 0.9f, 0.1f)) {}

	virtual IPcmData::WaveFormType getWaveFormType() const override { return IPcmData::WaveFormType::SquareWave; }
	virtual const char* getWaveFormTypeName() const override { return IWaveGenerator::SquareWaveFormTypeName; }
	virtual void generate(T* cycleData, size_t samplesPerCycle, WORD channels, float level) override;

protected:
	const float m_duty;
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
	for(; pos < samplesPerCycle; pos += channels) {
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
 * If ((peakPosition <= 0.0f) || (1.0f <= peakPosition) || (0.5f == peakPosition)), generate() method generates Sawtooth wave.
 */
template<typename T>
class TriangleWaveGenerator : public WaveGenerator<T>
{
public:
	TriangleWaveGenerator(float peakPosition) : m_peakPosition(limit(peakPosition)) {}

	virtual IPcmData::WaveFormType getWaveFormType() const override { return IPcmData::WaveFormType::TriangleWave; }
	virtual const char* getWaveFormTypeName() const override { return IWaveGenerator::TriangleWaveFormTypeName; }
	virtual void generate(T* cycleData, size_t samplesPerCycle, WORD channels, float level) override;

protected:
	const float m_peakPosition;
};

template<typename T>
void TriangleWaveGenerator<T>::generate(T* cycleData, size_t samplesPerCycle, WORD channels, float level)
{
	T highValue, lowValue, zeroValue;
	WaveGenerator<T>::adjustLevel(level, &highValue, &lowValue, &zeroValue);
	double height = (double)highValue - (double)lowValue;
	double upDelta, downDelta;
	upDelta = downDelta = height * channels / samplesPerCycle;
	bool up;
	double value;
	if(m_peakPosition == 0.0f) {
		upDelta = height;
		up = false;
		value = highValue;
	} else if(m_peakPosition == 1.0f) {
		downDelta = height;
		up = true;
		value = lowValue;
	} else if(m_peakPosition == 0.5f) {
		downDelta = height;
		up = true;
		value = zeroValue;
	} else if(m_peakPosition < 0.5f) {
		upDelta /= (m_peakPosition * 2);
		downDelta /= (1.0f - (m_peakPosition * 2));
		up = true;
		value = zeroValue;
	} else {
		upDelta /= (1.0f - ((1 - m_peakPosition) * 2));
		downDelta /= ((1 - m_peakPosition) * 2);
		up = false;
		value = zeroValue;
	}

	if((lowValue + upDelta) > highValue) { upDelta = height; }
	if((highValue - downDelta) < lowValue) { downDelta = height; }

	for(size_t pos = 0; pos < samplesPerCycle; pos += channels) {
		cycleData[pos] = (T)value;
		if(up) {
			value += upDelta;
			if(highValue <= value) {
				value = highValue;
				up = false;
			}
		}
		else {
			value -= downDelta;
			if(value <= lowValue) {
				value = lowValue;
				up = true;
			}
		}
	}
}
