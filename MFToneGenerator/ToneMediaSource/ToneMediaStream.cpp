#include "pch.h"

#include "ToneMediaStream.h"
#include "ToneMediaSource.h"


#pragma region Implementation of IMFMediaEventGenerator

ToneMediaStream::ToneMediaStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd)
	: m_mediaSource(mediaSource), m_sd(sd), m_sampleTime(0), m_key(220), m_unknownImpl(this)
{
}

// Note: MEStreamStarted Event is sent by ToneMediaSource::start().
HRESULT ToneMediaStream::start()
{
	m_sampleTime = 0;
	return S_OK;
}

// Note: MEStreamStopped Event is sent by ToneMediaSource::stop().
HRESULT ToneMediaStream::stop()
{
	return S_OK;
}

HRESULT ToneMediaStream::shutdown()
{
	m_eventGenerator.shutdown();

	return S_OK;
}

HRESULT __stdcall ToneMediaStream::GetMediaSource(__RPC__deref_out_opt IMFMediaSource** ppMediaSource)
{
	HR_ASSERT(ppMediaSource, E_POINTER);

	*ppMediaSource = m_mediaSource;
	m_mediaSource->AddRef();
	return S_OK;
}

HRESULT __stdcall ToneMediaStream::GetStreamDescriptor(__RPC__deref_out_opt IMFStreamDescriptor** ppStreamDescriptor)
{
	return HR_EXPECT_OK(m_sd.CopyTo(ppStreamDescriptor));
}

HRESULT __stdcall ToneMediaStream::RequestSample(IUnknown* pToken)
{
	CComPtr<IMFMediaTypeHandler> mh;
	m_sd->GetMediaTypeHandler(&mh);
	CComPtr<IMFMediaType> mediaType;
	mh->GetCurrentMediaType(&mediaType);
	CComHeapPtr<WAVEFORMATEX> pWaveFormat;
	UINT32 size;
	HR_ASSERT_OK(MFCreateWaveFormatExFromMFMediaType(mediaType, &pWaveFormat, &size, MFWaveFormatExConvertFlag_Normal));

	// Sample duration in mSec.
	static const UINT32 duration = 200;

	if(!m_waveGenerator) {
		m_waveGenerator.reset(new Square16bpsWaveGenerator((WORD)pWaveFormat->nSamplesPerSec, pWaveFormat->nChannels));
		m_waveGenerator->generate(m_key);
	}

	// Check WAVEFORMATEX retrieved from Stream Desctiptor whether it matches properties of Wave Generator.
	HR_ASSERT(pWaveFormat->wFormatTag == m_waveGenerator->getFormatTag(), MF_E_NOT_AVAILABLE);
	HR_ASSERT(pWaveFormat->wBitsPerSample == m_waveGenerator->getBitsPerSample(), MF_E_NOT_AVAILABLE);
	HR_ASSERT(pWaveFormat->nBlockAlign == m_waveGenerator->getBlockAlign(), MF_E_NOT_AVAILABLE);

	// Copy PCM data generated by WaveGenerator to IMFMediaBuffer.
	CComPtr<IMFMediaBuffer> buffer;
	m_waveGenerator->copyTo(&buffer, duration);

	// Create IMFSample contains the buffer.
	CComPtr<IMFSample> sample;
	MFCreateSample(&sample);
	sample->AddBuffer(buffer);
	if(pToken) {
		sample->SetUnknown(MFSampleExtension_Token, pToken);
	}
	LONGLONG sampleDuration = (LONGLONG)duration * 10000;	// 100-nanosecond units.
	sample->SetSampleDuration(sampleDuration);
	sample->SetSampleTime(m_sampleTime);
	m_sampleTime += sampleDuration;

	// Send MEMediaSample Event with the sample as Event Value.
	PROPVARIANT value = { VT_UNKNOWN };
	value.punkVal = sample;
	QueueEvent(MEMediaSample, &value);
	return S_OK;
}

HRESULT __stdcall ToneMediaStream::GetEvent(DWORD dwFlags, __RPC__deref_out_opt IMFMediaEvent** ppEvent)
{
	return m_eventGenerator.GetEvent(dwFlags, ppEvent);
}
HRESULT __stdcall ToneMediaStream::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	return m_eventGenerator.BeginGetEvent(pCallback, punkState);
}
HRESULT __stdcall ToneMediaStream::EndGetEvent(IMFAsyncResult* pResult, _Out_  IMFMediaEvent** ppEvent)
{
	return m_eventGenerator.EndGetEvent(pResult, ppEvent);
}
HRESULT __stdcall ToneMediaStream::QueueEvent(MediaEventType met, __RPC__in REFGUID guidExtendedType, HRESULT hrStatus, __RPC__in_opt const PROPVARIANT* pvValue)
{
	return m_eventGenerator.QueueEvent(met, guidExtendedType, hrStatus, pvValue);
}

#pragma endregion

#pragma region Implementation of IUnknown

HRESULT __stdcall ToneMediaStream::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject)
{
	HR_ASSERT(ppvObject, E_POINTER);

	static const QITAB qitab[] = {
		QITABENT(ToneMediaStream, IMFMediaStream),
		QITABENT(ToneMediaStream, IMFMediaEventGenerator),
	};
	return m_unknownImpl.QueryInterface(riid, ppvObject, qitab);
}

ULONG __stdcall ToneMediaStream::AddRef(void)
{
	return m_unknownImpl.AddRef();
}

ULONG __stdcall ToneMediaStream::Release(void)
{
	return m_unknownImpl.Release();
}

#pragma endregion
