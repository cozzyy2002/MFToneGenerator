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

	virtual void generate(T* cycleData, size_t cycleSize, WORD channels, float level) = 0;

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
		, m_cycleSize(0), m_currentPosition(0)
		, m_unknownImpl(this) {}

	virtual HRESULT copyTo(BYTE* destBuffer, size_t destSize) override;
	virtual void generate(float key, float level = DefaultLevel) override;

	virtual WORD getFormatTag() const { return FormatTag; }
	// Returns byte size of the minimum atomic unit of data to be generated.
	virtual size_t getBlockAlign() const override { return m_channels * sizeof(T); }
	virtual size_t getBitsPerSample() const override { return sizeof(T) * 8; }
	virtual WORD getSamplesPerSec() const override { return m_samplesPerSec; }
	virtual WORD getChannels() const override { return m_channels; }
	virtual size_t getSampleBufferSize(size_t duration) const { return m_samplesPerSec * getBlockAlign() * duration / 1000; }

	static const WORD FormatTag;
	static const T HighValue;
	static const T ZeroValue;
	static const T LowValue;

protected:
	const WORD m_samplesPerSec;
	const WORD m_channels;
	size_t m_cycleSize;
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
		if(m_cycleSize < m_currentPosition) { m_currentPosition = 0; }
	}

	return S_OK;
}

template<typename T>
void PcmData<T>::generate(float key, float level)
{
	auto cycleSize = (size_t)(m_channels * m_samplesPerSec / key);
	auto blockAlign = getBlockAlign();
	// Round up buffer size to block align boundary
	cycleSize = ((cycleSize / blockAlign) + ((cycleSize % blockAlign) ? 1 : 0)) * blockAlign;
	std::unique_ptr<T[]> cycleData(new T[cycleSize]);
	m_waveGenerator->generate(cycleData.get(), cycleSize, m_channels, level);

	// Update member variables in the Critical Section.
	{
		CriticalSection lock(m_cycleDataLock);
		m_cycleData = std::move(cycleData);
		m_cycleSize = cycleSize;
		m_currentPosition = 0;
	}
}


/*
 * SquareWaveGenerator class derived from WaveGenerator class.
 * 
 * Override of WaveGenerator::generate() method generates Square wave.
 * This class exposes generate() method that has duty parameter along with above.
 */
template<typename T>
class SquareWaveGenerator : public WaveGenerator<T>
{
public:
	SquareWaveGenerator(float duty) : m_duty(duty) {}

	virtual void generate(T* cycleData, size_t cycleSize, WORD channels, float level) override;

protected:
	float m_duty;
};

template<typename T>
void SquareWaveGenerator<T>::generate(T* cycleData, size_t cycleSize, WORD channels, float level)
{
	auto highValue = (T)(PcmData<T>::HighValue * level);
	auto lowValue = (T)(PcmData<T>::LowValue * level);
	size_t highDuration = (size_t)(cycleSize * m_duty);
	size_t pos = 0;
	for(; pos < highDuration; pos += channels) {
		for(size_t ch = 0; ch < channels; ch++) {
			cycleData[pos + ch] = highValue;
		}
	}
	for(; pos < cycleSize; pos++) {
		for(size_t ch = 0; ch < channels; ch++) {
			cycleData[pos + ch] = lowValue;
		}
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
	virtual void generate(T* cycleData, size_t cycleSize, WORD channels, float level) override;
};

template<typename T>
void SineWaveGenerator<T>::generate(T* cycleData, size_t cycleSize, WORD channels, float level)
{
	static const float pi = 3.141592f;
	auto highValue = (T)((PcmData<T>::HighValue - PcmData<T>::ZeroValue) * level);

	for(size_t pos = 0; pos < cycleSize; pos += channels) {
		auto radian = 2 * pi * pos / cycleSize;
		auto value = (T)((sin(radian) * highValue) + PcmData<T>::ZeroValue);
		for(size_t ch = 0; ch < channels; ch++) {
			cycleData[pos + ch] = value;
		}
	}
}
