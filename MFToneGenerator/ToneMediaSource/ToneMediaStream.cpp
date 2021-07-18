#include "pch.h"

#include "ToneMediaStream.h"
#include "ToneMediaSource.h"


#pragma region Implementation of IMFMediaEventGenerator

ToneMediaStream::ToneMediaStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd)
	: m_mediaSource(mediaSource), m_sd(sd), m_unknownImpl(this)
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
	m_mediaSource.Release();

	return S_OK;
}

HRESULT __stdcall ToneMediaStream::GetMediaSource(__RPC__deref_out_opt IMFMediaSource** ppMediaSource)
{
	HR_ASSERT(ppMediaSource, E_POINTER);

	HR_ASSERT_OK(m_mediaSource.QueryInterface(ppMediaSource));
	return S_OK;
}

HRESULT __stdcall ToneMediaStream::GetStreamDescriptor(__RPC__deref_out_opt IMFStreamDescriptor** ppStreamDescriptor)
{
	HR_ASSERT(ppStreamDescriptor, E_POINTER);

	HR_ASSERT_OK(m_sd.CopyTo(ppStreamDescriptor));
	return S_OK;
}

HRESULT __stdcall ToneMediaStream::RequestSample(IUnknown* pToken)
{
	CComPtr<IMFMediaTypeHandler> mh;
	m_sd->GetMediaTypeHandler(&mh);
	CComPtr<IMFMediaType> mediaType;
	mh->GetCurrentMediaType(&mediaType);
	WAVEFORMATEX* pWaveFormat;
	UINT32 size;
	HR_ASSERT_OK(MFCreateWaveFormatExFromMFMediaType(mediaType, &pWaveFormat, &size, MFWaveFormatExConvertFlag_Normal));

	// Only mono, 16bit PCM format is supported.
	HR_ASSERT(pWaveFormat->wFormatTag == WAVE_FORMAT_PCM, E_UNEXPECTED);
	HR_ASSERT(pWaveFormat->nChannels == 1, E_UNEXPECTED);
	HR_ASSERT(pWaveFormat->wBitsPerSample == 16, E_UNEXPECTED);

	// Sample duration in mSec.
	static const UINT32 duration = 80;

	// Create IMFSample that has enough size to hold PCM audio data.
	DWORD bufferSize =
		pWaveFormat->nChannels *
		pWaveFormat->nSamplesPerSec * duration / 1000 *
		pWaveFormat->wBitsPerSample / 8;
	CComPtr<IMFMediaBuffer> buffer;
	MFCreateMemoryBuffer(bufferSize, &buffer);

	// Generate PCM data.

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
	value.punkVal = sample.Detach();
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
