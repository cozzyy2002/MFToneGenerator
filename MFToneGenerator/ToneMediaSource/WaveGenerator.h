#pragma once

/*
 * IWaveGenerator interface.
 */
class IWaveGenerator
{
public:
	static const float DefaultLevel;

	virtual HRESULT copyTo(IMFMediaBuffer** pDest, DWORD duration) = 0;
	virtual HRESULT copyTo(IMFMediaBuffer* dest) = 0;
	virtual void generate(float key, float level = DefaultLevel) = 0;

	virtual WORD getFormatTag() const = 0;
	virtual size_t getBlockAlign() const = 0;
	virtual size_t getBitsPerSample() const = 0;
	virtual WORD getSamplesPerSec() const = 0;
	virtual WORD getChannels() const = 0;
};

/*
 * WaveGenerator template abstract class
 * Type parameter T is data type of wave sample data.
 *   Such as T=UINT16 for 16 bit per sample per channel.
 *   This case, minimum atomic unit of 2-channel sample is 16 / 8 * 2 byte.
 * 
 * IWaveGenerator::generate() and getFormatTag() methods should be implemented by derived class.
 */
template<typename T>
class WaveGenerator : public IWaveGenerator
{
public:
	WaveGenerator(WORD samplesPerSec, WORD channels)
		: m_samplesPerSec(samplesPerSec), m_channels(channels), m_cycleSize(0), m_currentPosition(0) {}
	virtual ~WaveGenerator() {}

	HRESULT copyTo(IMFMediaBuffer** pDest, DWORD duration);
	HRESULT copyTo(IMFMediaBuffer* dest);

	// Returns byte size of the minimum atomic unit of data to be generated.
	virtual size_t getBlockAlign() const override { return m_channels * sizeof(T); }
	virtual size_t getBitsPerSample() const override { return sizeof(T) * 8; }
	virtual WORD getSamplesPerSec() const override { return m_samplesPerSec; }
	virtual WORD getChannels() const override { return m_channels; }

	operator bool() const { return (bool)m_cycleData; }

protected:
	// Allocate buffer enough to hold 1-cycle PCM data.
	void allocateCycleData(float key);

	T getHighValue() const;
	T getLowValue() const;

	const WORD m_samplesPerSec;
	const WORD m_channels;
	size_t m_cycleSize;
	size_t m_currentPosition;
	std::unique_ptr<T[]> m_cycleData;
};

template<typename T>
HRESULT WaveGenerator<T>::copyTo(IMFMediaBuffer** pDest, DWORD duration)
{
	DWORD size = m_samplesPerSec * duration / 1000 * getBlockAlign();
	HR_ASSERT_OK(MFCreateMemoryBuffer(size, pDest));
	return copyTo(*pDest);
}

template<typename T>
HRESULT WaveGenerator<T>::copyTo(IMFMediaBuffer* dest)
{
	// Assert that data has been generated.
	HR_ASSERT(m_cycleData, E_ILLEGAL_METHOD_CALL);

	BYTE* destBuffer;
	DWORD destSize;
	HR_ASSERT_OK(dest->Lock(&destBuffer, &destSize, nullptr));
	auto blockAlign = getBlockAlign();
	destSize = (destSize / blockAlign) * blockAlign;
	for(DWORD destPosition = 0; destPosition < destSize; destPosition += sizeof(T)) {
		*(T*)(&destBuffer[destPosition]) = m_cycleData[m_currentPosition++];
		if(m_cycleSize < m_currentPosition) { m_currentPosition = 0; }
	}
	HR_ASSERT_OK(dest->Unlock());
	HR_ASSERT_OK(dest->SetCurrentLength(destSize));

	return S_OK;
}

template<typename T>
void WaveGenerator<T>::allocateCycleData(float key)
{
	m_currentPosition = 0;
	m_cycleSize = (size_t)(m_channels * m_samplesPerSec / key);
	auto blockAlign = getBlockAlign();
	m_cycleSize = (m_cycleSize / blockAlign) * blockAlign;		// Adjust buffer size to block align boundary
	m_cycleData.reset(new T[m_cycleSize]);
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
	static const float DefaultDuty;

	SquareWaveGenerator(WORD samplesPerSec, WORD channels) : WaveGenerator(samplesPerSec, channels) {}

	virtual void generate(float key, float level = DefaultLevel) override;
	void generate(float key, float level, float duty);
};

template<typename T>
void SquareWaveGenerator<T>::generate(float key, float level /*= DefaultLevel*/)
{
	generate(key, level, DefaultDuty);
}

template<typename T>
void SquareWaveGenerator<T>::generate(float key, float level, float duty)
{
	allocateCycleData(key);

	T highValue = (T)(getHighValue() * level);
	T lowValue = (T)(getLowValue() * level);
	size_t highDuration = (size_t)(m_cycleSize * duty);
	size_t pos = 0;
	for(; pos < highDuration; pos++) {
		for(size_t ch = 0; ch < m_channels; ch++) {
			m_cycleData[pos + ch] = highValue;
		}
	}
	for(; pos < m_cycleSize; pos++) {
		for(size_t ch = 0; ch < m_channels; ch++) {
			m_cycleData[pos + ch] = lowValue;
		}
	}
}

template<typename T>
/*static*/ const float SquareWaveGenerator<T>::DefaultDuty = 0.5f;

class Square8bpsWaveGenerator : public SquareWaveGenerator<UINT8>
{
public:
	Square8bpsWaveGenerator(WORD samplesPerSec, WORD channels) : SquareWaveGenerator(samplesPerSec, channels) {}

	virtual WORD getFormatTag() const override { return WAVE_FORMAT_PCM; }
};

class Square16bpsWaveGenerator : public SquareWaveGenerator<UINT16>
{
public:
	Square16bpsWaveGenerator(WORD samplesPerSec, WORD channels) : SquareWaveGenerator(samplesPerSec, channels) {}

	virtual WORD getFormatTag() const override { return WAVE_FORMAT_PCM; }
};
