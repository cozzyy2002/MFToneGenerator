#pragma once

class CriticalSection
{
public:
    class Object
    {
        friend class CriticalSection;
    public:
        Object() { InitializeCriticalSection(&m_criticalSection); }
        ~Object() { DeleteCriticalSection(&m_criticalSection); }

    protected:
        CRITICAL_SECTION* get() { return &m_criticalSection; }
        CRITICAL_SECTION m_criticalSection;
    };

    CriticalSection(Object& object) : m_object(object) { EnterCriticalSection(m_object.get()); }
    ~CriticalSection() { LeaveCriticalSection(m_object.get()); }

protected:
    Object& m_object;
};

/**
 * Custom Media Source generates Tone Wave.
 */
class ToneMediaSource : public IMFMediaSource
{
public:
    ToneMediaSource();

protected:
    HRESULT checkShutdown();

#pragma region Implementation of IMFMediaSource
public:
    virtual HRESULT STDMETHODCALLTYPE GetCharacteristics(
        /* [out] */ __RPC__out DWORD* pdwCharacteristics);
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreatePresentationDescriptor(
        /* [annotation][out] */
        _Outptr_  IMFPresentationDescriptor** ppPresentationDescriptor);
    virtual HRESULT STDMETHODCALLTYPE Start(
        /* [in] */ __RPC__in_opt IMFPresentationDescriptor* pPresentationDescriptor,
        /* [unique][in] */ __RPC__in_opt const GUID* pguidTimeFormat,
        /* [unique][in] */ __RPC__in_opt const PROPVARIANT* pvarStartPosition);
    virtual HRESULT STDMETHODCALLTYPE Stop(void);
    virtual HRESULT STDMETHODCALLTYPE Pause(void);
    virtual HRESULT STDMETHODCALLTYPE Shutdown(void);
#pragma endregion

#pragma region Implementation of IMFMediaEventGenerator
public:
    virtual HRESULT STDMETHODCALLTYPE GetEvent(
        /* [in] */ DWORD dwFlags,
        /* [out] */ __RPC__deref_out_opt IMFMediaEvent** ppEvent);
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE BeginGetEvent(
        /* [in] */ IMFAsyncCallback* pCallback,
        /* [in] */ IUnknown* punkState);
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE EndGetEvent(
        /* [in] */ IMFAsyncResult* pResult,
        /* [annotation][out] */
        _Out_  IMFMediaEvent** ppEvent);
    virtual HRESULT STDMETHODCALLTYPE QueueEvent(
        /* [in] */ MediaEventType met,
        /* [in] */ __RPC__in REFGUID guidExtendedType,
        /* [in] */ HRESULT hrStatus,
        /* [unique][in] */ __RPC__in_opt const PROPVARIANT* pvValue);

protected:
    CComPtr<IMFMediaEventQueue> m_eventQueue;
    CriticalSection::Object m_eventQueueLock;
#pragma endregion

#pragma region Implementation of IUnknown
public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

protected:
    tsm::UnknownImpl<ToneMediaSource> m_unknownImpl;
#pragma endregion
};
