#pragma once

#include "Utils.h"
#include <PcmData/PcmData.h>

/**
 * Custom MediaEvntGenerator implementation.
 */
class MediaEventGenerator
{
public:
    MediaEventGenerator();

    HRESULT shutdown();
    HRESULT checkShutdown();

    /*
        Queue MediaEvent with GUID_NULL as ExtendedType and S_OK as hrStatus
        If pvValue is not specified, Event Value is set VT_EMPTY.
    */
    HRESULT QueueEvent(MediaEventType met, const PROPVARIANT* pvValue = nullptr);

#pragma region Implementation of IMFMediaEventGenerator
public:
    HRESULT STDMETHODCALLTYPE GetEvent(
        /* [in] */ DWORD dwFlags,
        /* [out] */ __RPC__deref_out_opt IMFMediaEvent** ppEvent);
    HRESULT STDMETHODCALLTYPE BeginGetEvent(
        /* [in] */ IMFAsyncCallback* pCallback,
        /* [in] */ IUnknown* punkState);
    HRESULT STDMETHODCALLTYPE EndGetEvent(
        /* [in] */ IMFAsyncResult* pResult,
        /* [annotation][out] */
        _Out_  IMFMediaEvent** ppEvent);
    HRESULT STDMETHODCALLTYPE QueueEvent(
        /* [in] */ MediaEventType met,
        /* [in] */ __RPC__in REFGUID guidExtendedType,
        /* [in] */ HRESULT hrStatus,
        /* [unique][in] */ __RPC__in_opt const PROPVARIANT* pvValue);

protected:
    CComPtr<IMFMediaEventQueue> m_eventQueue;
    CriticalSection::Object m_eventQueueLock;
    static const PROPVARIANT m_nullValue;
#pragma endregion
};
