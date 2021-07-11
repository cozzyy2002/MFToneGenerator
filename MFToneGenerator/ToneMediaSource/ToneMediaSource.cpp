#include "pch.h"

#include "ToneMediaSource.h"

ToneMediaSource::ToneMediaSource()
	: m_unknownImpl(this)
{
	HR_EXPECT_OK(MFCreateEventQueue(&m_eventQueue));
}

HRESULT ToneMediaSource::checkShutdown()
{
	return m_eventQueue ? S_OK : MF_E_SHUTDOWN;
}

#pragma region Implementation of IMFMediaSource
HRESULT __stdcall ToneMediaSource::GetCharacteristics(__RPC__out DWORD* pdwCharacteristics)
{
	HR_ASSERT(pdwCharacteristics, E_POINTER);
	HR_ASSERT_OK(checkShutdown());

	return MFMEDIASOURCE_DOES_NOT_USE_NETWORK;
}

HRESULT __stdcall ToneMediaSource::CreatePresentationDescriptor(_Outptr_  IMFPresentationDescriptor** ppPresentationDescriptor)
{
	return E_NOTIMPL;
}

HRESULT __stdcall ToneMediaSource::Start(__RPC__in_opt IMFPresentationDescriptor* pPresentationDescriptor, __RPC__in_opt const GUID* pguidTimeFormat, __RPC__in_opt const PROPVARIANT* pvarStartPosition)
{
	return E_NOTIMPL;
}

HRESULT __stdcall ToneMediaSource::Stop(void)
{
	return E_NOTIMPL;
}

HRESULT __stdcall ToneMediaSource::Pause(void)
{
	return E_NOTIMPL;
}

HRESULT __stdcall ToneMediaSource::Shutdown(void)
{
	CriticalSection lock(m_eventQueueLock);
	HR_ASSERT_OK(checkShutdown());

	// Shutdown for IMFMediaEventGenarator implementation.
	HR_EXPECT_OK(m_eventQueue->Shutdown());
	m_eventQueue.Release();

	return S_OK;
}
#pragma endregion

// See https://docs.microsoft.com/en-us/windows/win32/medfound/media-event-generators
#pragma region Implementation of IMFMediaEventGenarator

HRESULT __stdcall ToneMediaSource::GetEvent(DWORD dwFlags, __RPC__deref_out_opt IMFMediaEvent** ppEvent)
{
	CComPtr<IMFMediaEventQueue> eventQueue;
	{
		// Get IMFMediaEventQueue in the CriticalSection.
		CriticalSection lock(m_eventQueueLock);
		HR_ASSERT_OK(checkShutdown());
		eventQueue = m_eventQueue;
	}

	return HR_EXPECT_OK(eventQueue->GetEvent(dwFlags, ppEvent));
}

HRESULT __stdcall ToneMediaSource::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	CriticalSection lock(m_eventQueueLock);
	HR_ASSERT_OK(checkShutdown());

	return HR_EXPECT_OK(m_eventQueue->BeginGetEvent(pCallback, punkState));
}

HRESULT __stdcall ToneMediaSource::EndGetEvent(IMFAsyncResult* pResult, _Out_  IMFMediaEvent** ppEvent)
{
	CriticalSection lock(m_eventQueueLock);
	HR_ASSERT_OK(checkShutdown());

	return HR_EXPECT_OK(m_eventQueue->EndGetEvent(pResult, ppEvent));
}

HRESULT __stdcall ToneMediaSource::QueueEvent(MediaEventType met, __RPC__in REFGUID guidExtendedType, HRESULT hrStatus, __RPC__in_opt const PROPVARIANT* pvValue)
{
	CriticalSection lock(m_eventQueueLock);
	HR_ASSERT_OK(checkShutdown());

	return HR_EXPECT_OK(m_eventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue));
}
#pragma endregion

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
