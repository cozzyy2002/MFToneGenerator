#include "pch.h"

#include "ToneMediaSource.h"
#include "ToneMediaStream.h"

ToneMediaSource::ToneMediaSource()
	: m_unknownImpl(this)
{
}

HRESULT ToneMediaSource::checkShutdown()
{
	HR_ASSERT_OK(m_eventGenerator.checkShutdown());

	return S_OK;
}

#pragma region Implementation of IMFMediaSource
HRESULT __stdcall ToneMediaSource::GetCharacteristics(__RPC__out DWORD* pdwCharacteristics)
{
	HR_ASSERT(pdwCharacteristics, E_POINTER);
	HR_ASSERT_OK(checkShutdown());

	*pdwCharacteristics = MFMEDIASOURCE_DOES_NOT_USE_NETWORK;
	return S_OK;
}

HRESULT __stdcall ToneMediaSource::CreatePresentationDescriptor(_Outptr_ IMFPresentationDescriptor** ppPresentationDescriptor)
{
	HR_ASSERT(ppPresentationDescriptor, E_POINTER);
	HR_ASSERT_OK(checkShutdown());

	if(!m_pd) {
		// Create MediaType
		static const WORD nChannels = 1;			// Mono
		static const DWORD nSamplesPerSec = 44100;	// 44.1kHz
		static const WORD wBitsPerSample = 16;		// 16bit/Sample
		WAVEFORMATEX waveFormat = {
			WAVE_FORMAT_PCM,
			nChannels,											// nChannels
			nSamplesPerSec,										// nSamplesPerSec
			nChannels * nSamplesPerSec * wBitsPerSample / 8,	// nAvgBytesPerSec
			nChannels * wBitsPerSample / 8,						// nBlockAlign
			wBitsPerSample,										// wBitsPerSample
			0,													// cbSize(No extra information)
		};

		CComPtr<IMFMediaType> mediaType;
		MFCreateMediaType(&mediaType);
		MFInitMediaTypeFromWaveFormatEx(mediaType, &waveFormat, sizeof(waveFormat));

		// Create StreamDesctiptor and set current MediaType
		CComPtr<IMFStreamDescriptor> sd;
		IMFMediaType* mediaTypes[] = { mediaType.p };
		MFCreateStreamDescriptor(1, ARRAYSIZE(mediaTypes), mediaTypes, &sd);
		CComPtr<IMFMediaTypeHandler> mth;
		sd->GetMediaTypeHandler(&mth);
		mth->SetCurrentMediaType(mediaType);

		// Create PresentationDesctiptor and select the only MediaStream.
		IMFStreamDescriptor* sds[] = { sd.p };
		MFCreatePresentationDescriptor(ARRAYSIZE(sds), sds, &m_pd);
		m_pd->SelectStream(0);
	}

	m_pd->Clone(ppPresentationDescriptor);

	return S_OK;
}

HRESULT __stdcall ToneMediaSource::Start(__RPC__in_opt IMFPresentationDescriptor* pPresentationDescriptor, __RPC__in_opt const GUID* pguidTimeFormat, __RPC__in_opt const PROPVARIANT* pvarStartPosition)
{
	HR_ASSERT(!m_mediaStream, E_ILLEGAL_METHOD_CALL);

	CComPtr<IMFStreamDescriptor> sd;
	BOOL selected;
	HR_ASSERT_OK(pPresentationDescriptor->GetStreamDescriptorByIndex(0, &selected, &sd));
	HR_ASSERT(selected, E_UNEXPECTED);

	m_mediaStream = new ToneMediaStream(this, sd);

	PROPVARIANT value = { VT_UNKNOWN };
	HR_ASSERT_OK(m_mediaStream.QueryInterface(&value.punkVal));
	m_eventGenerator.QueueEvent(MENewStream, &value);
	m_eventGenerator.QueueEvent(MESourceStarted, pvarStartPosition);

	m_mediaStream->start();
	m_mediaStream->QueueEvent(MEStreamStarted, pvarStartPosition);

	return S_OK;
}

HRESULT __stdcall ToneMediaSource::Stop(void)
{
	m_eventGenerator.QueueEvent(MESourceStopped);

	m_mediaStream->stop();
	m_mediaStream->QueueEvent(MEStreamStopped);

	return S_OK;
}

HRESULT __stdcall ToneMediaSource::Pause(void)
{
	return E_NOTIMPL;
}

HRESULT __stdcall ToneMediaSource::Shutdown(void)
{
	m_eventGenerator.shutdown();

	if(m_mediaStream) {
		m_mediaStream->shutdown();
		m_mediaStream.Release();
	}

	return S_OK;
}

#pragma region Implementation of IMFMediaEventGenerator

HRESULT __stdcall ToneMediaSource::GetEvent(DWORD dwFlags, __RPC__deref_out_opt IMFMediaEvent** ppEvent)
{
	return m_eventGenerator.GetEvent(dwFlags, ppEvent);
}
HRESULT __stdcall ToneMediaSource::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	return m_eventGenerator.BeginGetEvent(pCallback, punkState);
}
HRESULT __stdcall ToneMediaSource::EndGetEvent(IMFAsyncResult* pResult, _Out_  IMFMediaEvent** ppEvent)
{
	return m_eventGenerator.EndGetEvent(pResult, ppEvent);
}
HRESULT __stdcall ToneMediaSource::QueueEvent(MediaEventType met, __RPC__in REFGUID guidExtendedType, HRESULT hrStatus, __RPC__in_opt const PROPVARIANT* pvValue)
{
	return m_eventGenerator.QueueEvent(met, guidExtendedType, hrStatus, pvValue);
}

#pragma endregion

#pragma region Implementation of IUnknown

HRESULT __stdcall ToneMediaSource::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject)
{
	HR_ASSERT(ppvObject, E_POINTER);

	static const QITAB qitab[] = {
		QITABENT(ToneMediaSource, IMFMediaSource),
		QITABENT(ToneMediaSource, IMFMediaEventGenerator),
	};
	return m_unknownImpl.QueryInterface(riid, ppvObject, qitab);
}

ULONG __stdcall ToneMediaSource::AddRef(void)
{
	return m_unknownImpl.AddRef();
}

ULONG __stdcall ToneMediaSource::Release(void)
{
	return m_unknownImpl.Release();
}

#pragma endregion
