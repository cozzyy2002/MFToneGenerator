#include "pch.h"

#include "MediaEventGenerator.h"

// See https://docs.microsoft.com/en-us/windows/win32/medfound/media-event-generators
#pragma region Implementation of IMFMediaEventGenarator

MediaEventGenerator::MediaEventGenerator()
{
	HR_EXPECT_OK(MFCreateEventQueue(&m_eventQueue));
}

HRESULT MediaEventGenerator::shutdown()
{
	CriticalSection lock(m_eventQueueLock);

	if(m_eventQueue) {
		// Shutdown for IMFMediaEventGenarator implementation.
		HR_EXPECT_OK(m_eventQueue->Shutdown());
		m_eventQueue.Release();
	}

	return S_OK;
}

HRESULT MediaEventGenerator::checkShutdown()
{
	return m_eventQueue ? S_OK : MF_E_SHUTDOWN;
}

/*static*/ const PROPVARIANT MediaEventGenerator::m_nullValue = { VT_EMPTY };

HRESULT MediaEventGenerator::QueueEvent(MediaEventType met, const PROPVARIANT* pvValue /*= nullptr*/)
{
	return HR_EXPECT_OK(QueueEvent(met, GUID_NULL, S_OK, pvValue ? pvValue : &m_nullValue));
}

HRESULT __stdcall MediaEventGenerator::GetEvent(DWORD dwFlags, __RPC__deref_out_opt IMFMediaEvent** ppEvent)
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

HRESULT __stdcall MediaEventGenerator::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	CriticalSection lock(m_eventQueueLock);
	HR_ASSERT_OK(checkShutdown());

	return HR_EXPECT_OK(m_eventQueue->BeginGetEvent(pCallback, punkState));
}

HRESULT __stdcall MediaEventGenerator::EndGetEvent(IMFAsyncResult* pResult, _Out_  IMFMediaEvent** ppEvent)
{
	CriticalSection lock(m_eventQueueLock);
	HR_ASSERT_OK(checkShutdown());

	return HR_EXPECT_OK(m_eventQueue->EndGetEvent(pResult, ppEvent));
}

HRESULT __stdcall MediaEventGenerator::QueueEvent(MediaEventType met, __RPC__in REFGUID guidExtendedType, HRESULT hrStatus, __RPC__in_opt const PROPVARIANT* pvValue)
{
	CriticalSection lock(m_eventQueueLock);
	if(FAILED(checkShutdown())) {
		Logger logger;
		logger.log(_T("Warning: Trying to Queue event (met=%d, GUID=%s, hr=0x%p, pv->vt=%d) after shutdown."),
			met, logger(guidExtendedType).c_str(), hrStatus, pvValue ? pvValue->vt : 0xff);
		return S_FALSE;
	}

	return HR_EXPECT_OK(m_eventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue));
}
#pragma endregion
