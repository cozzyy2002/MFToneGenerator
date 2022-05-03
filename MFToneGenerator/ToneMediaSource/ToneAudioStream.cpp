#include "pch.h"

#include "ToneAudioStream.h"

ToneAudioStream::ToneAudioStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd, std::shared_ptr<IPcmData>& pcmData)
	: ToneMediaStream(mediaSource, sd), m_pcmData(pcmData), m_key(0)
{
}

/*static*/ HRESULT ToneAudioStream::createStreamDescriptor(IPcmData* pPcmData, DWORD streamId, IMFStreamDescriptor** ppsd)
{
	const DWORD nSamplesPerSec = pPcmData->getSamplesPerSec();
	const WORD nBlockAlign = pPcmData->getBlockAlign();
	WAVEFORMATEX waveFormat = {
		pPcmData->getFormatTag(),			// wFormatTag
		pPcmData->getChannels(),			// nChannels
		nSamplesPerSec,						// nSamplesPerSec
		nSamplesPerSec * nBlockAlign,		// nAvgBytesPerSec
		nBlockAlign,						// nBlockAlign
		pPcmData->getBitsPerSample(),		// wBitsPerSample
		0,									// cbSize(No extra information)
	};

	CComPtr<IMFMediaType> mediaType;
	HR_ASSERT_OK(MFCreateMediaType(&mediaType));
	HR_ASSERT_OK(MFInitMediaTypeFromWaveFormatEx(mediaType, &waveFormat, sizeof(waveFormat)));

	IMFMediaType* mediaTypes[] = { mediaType.p };
	return ToneMediaStream::createStreamDescriptor(mediaTypes, streamId, ppsd);
}

HRESULT __stdcall ToneAudioStream::RequestSample(IUnknown* pToken)
{
	if (!m_pcmData) { return S_FALSE; }

	// Sample duration in mSec.
	static const UINT32 duration = 200;

	// Copy PCM data generated by WaveGenerator to IMFMediaBuffer.
	CComPtr<IMFMediaBuffer> buffer;
	BYTE* rawBuffer;
	DWORD size = m_pcmData->getSampleBufferSize(duration);
	HR_ASSERT_OK(MFCreateMemoryBuffer(size, &buffer));
	HR_ASSERT_OK(buffer->Lock(&rawBuffer, nullptr, nullptr));
	HR_ASSERT_OK(m_pcmData->copyTo(rawBuffer, size));
	HR_ASSERT_OK(buffer->Unlock());
	HR_ASSERT_OK(buffer->SetCurrentLength(size));

	// Create IMFSample contains the buffer.
	CComPtr<IMFSample> sample;
	HR_ASSERT_OK(MFCreateSample(&sample));
	sample->AddBuffer(buffer);
	if (pToken) {
		sample->SetUnknown(MFSampleExtension_Token, pToken);
	}
	LONGLONG sampleDuration = (LONGLONG)duration * 10000;	// 100-nanosecond units.
	sample->SetSampleDuration(sampleDuration);
	sample->SetSampleTime(m_sampleTime);
	m_sampleTime += sampleDuration;

	// Send MEMediaSample Event with the sample as Event Value.
	PROPVARIANT value = { VT_UNKNOWN };
	value.punkVal = sample;
	m_eventGenerator.QueueEvent(MEMediaSample, &value);
	return S_OK;
}

HRESULT ToneAudioStream::onStart(const PROPVARIANT* pvarStartPosition)
{
	m_sampleTime = 0;

	CComPtr<IMFMediaTypeHandler> mh;
	m_sd->GetMediaTypeHandler(&mh);
	CComPtr<IMFMediaType> mediaType;
	mh->GetCurrentMediaType(&mediaType);
	CComHeapPtr<WAVEFORMATEX> pWaveFormat;
	UINT32 size;
	HR_ASSERT_OK(MFCreateWaveFormatExFromMFMediaType(mediaType, &pWaveFormat, &size, MFWaveFormatExConvertFlag_Normal));

	// Check WAVEFORMATEX retrieved from Stream Desctiptor whether it matches properties of Wave Generator.
	HR_ASSERT(pWaveFormat->wFormatTag == m_pcmData->getFormatTag(), MF_E_NOT_AVAILABLE);
	HR_ASSERT(pWaveFormat->wBitsPerSample == m_pcmData->getBitsPerSample(), MF_E_NOT_AVAILABLE);
	HR_ASSERT(pWaveFormat->nBlockAlign == m_pcmData->getBlockAlign(), MF_E_NOT_AVAILABLE);

	return S_OK;
}

HRESULT ToneAudioStream::onStop()
{
	// Nothing to do.
	return S_OK;
}

HRESULT ToneAudioStream::onShutdown()
{
	m_pcmData.reset();

	return S_OK;
}
