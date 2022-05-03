#include "pch.h"

#include "ToneMediaStream.h"
#include "ToneMediaSource.h"


#pragma region Implementation of IMFMediaEventGenerator

ToneMediaStream::ToneMediaStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd)
	: m_mediaSource(mediaSource), m_sd(sd), m_sampleTime(0), m_unknownImpl(this)
{
}

HRESULT ToneMediaStream::start(const PROPVARIANT* pvarStartPosition)
{
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
