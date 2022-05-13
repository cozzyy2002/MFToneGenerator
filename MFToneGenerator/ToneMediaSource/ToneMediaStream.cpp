#include "pch.h"

#include "ToneMediaStream.h"
#include "ToneMediaSource.h"


ToneMediaStream::ToneMediaStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd)
	: m_mediaSource(mediaSource), m_sd(sd), m_sampleTime(0), m_unknownImpl(this)
{
}

HRESULT __stdcall ToneMediaStream::RequestSample(IUnknown* pToken)
{
	// Create IMFSample contains the buffer.
	CComPtr<IMFSample> sample;
	HR_ASSERT_OK(MFCreateSample(&sample));
	if (pToken) {
		sample->SetUnknown(MFSampleExtension_Token, pToken);
	}
	HR_ASSERT_OK(onRequestSample(sample));

	// Send MEMediaSample Event with the sample as Event Value.
	PROPVARIANT value = { VT_UNKNOWN };
	value.punkVal = sample;
	m_eventGenerator.QueueEvent(MEMediaSample, &value);
	return S_OK;
}

HRESULT ToneMediaStream::start(const PROPVARIANT* pvarStartPosition)
{
	CComPtr<IMFMediaTypeHandler> mth;
	HR_ASSERT_OK(m_sd->GetMediaTypeHandler(&mth));
	m_mediaType.Release();
	HR_ASSERT_OK(mth->GetCurrentMediaType(&m_mediaType));
	HR_ASSERT_OK(onStart(pvarStartPosition));

	m_eventGenerator.QueueEvent(MEStreamStarted, pvarStartPosition);

	return S_OK;
}

HRESULT ToneMediaStream::stop()
{
	HR_ASSERT_OK(onStop());

	m_eventGenerator.QueueEvent(MEStreamStopped);

	return S_OK;
}

HRESULT ToneMediaStream::shutdown()
{
	HR_EXPECT_OK(onShutdown());

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

#pragma region Implementation of IMFMediaEventGenerator

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
		{ 0 }
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
