#pragma once

#include "MediaEventGenerator.h"
#include "PcmData/PcmData.h"
#include "Utils.h"

class ToneMediaStream;

/**
 * Custom Media Source generates Tone Wave.
 */
class ToneMediaSource : public IMFMediaSource, DoNotCopy
{
public:
    ToneMediaSource(std::shared_ptr<IPcmData>& pcmData);

protected:
    HRESULT checkShutdown();
    std::shared_ptr<IPcmData> m_pcmData;

#pragma region Implementation of IMFMediaSource
public:
    virtual HRESULT STDMETHODCALLTYPE GetCharacteristics(
        /* [out] */ __RPC__out DWORD* pdwCharacteristics) override;
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreatePresentationDescriptor(
        /* [annotation][out] */
        _Outptr_  IMFPresentationDescriptor** ppPresentationDescriptor) override;
    virtual HRESULT STDMETHODCALLTYPE Start(
        /* [in] */ __RPC__in_opt IMFPresentationDescriptor* pPresentationDescriptor,
        /* [unique][in] */ __RPC__in_opt const GUID* pguidTimeFormat,
        /* [unique][in] */ __RPC__in_opt const PROPVARIANT* pvarStartPosition) override;
    virtual HRESULT STDMETHODCALLTYPE Stop(void) override;
    virtual HRESULT STDMETHODCALLTYPE Pause(void) override;
    virtual HRESULT STDMETHODCALLTYPE Shutdown(void) override;

protected:
    enum class StreamId {
        ToneAudio = 1,
        ToneVideo = 2
    };
    CComPtr<IMFPresentationDescriptor> m_pd;
    std::vector<CComPtr<ToneMediaStream>> m_mediaStreams;
#pragma endregion

#pragma region Implementation of IMFMediaEventGenerator
public:
    virtual HRESULT STDMETHODCALLTYPE GetEvent(
        /* [in] */ DWORD dwFlags,
        /* [out] */ __RPC__deref_out_opt IMFMediaEvent** ppEvent) override;
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE BeginGetEvent(
        /* [in] */ IMFAsyncCallback* pCallback,
        /* [in] */ IUnknown* punkState) override;
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE EndGetEvent(
        /* [in] */ IMFAsyncResult* pResult,
        /* [annotation][out] */ _Out_  IMFMediaEvent** ppEvent) override;
    virtual HRESULT STDMETHODCALLTYPE QueueEvent(
        /* [in] */ MediaEventType met,
        /* [in] */ __RPC__in REFGUID guidExtendedType,
        /* [in] */ HRESULT hrStatus,
        /* [unique][in] */ __RPC__in_opt const PROPVARIANT* pvValue) override;

protected:
    MediaEventGenerator m_eventGenerator;
#pragma endregion

#pragma region Implementation of IUnknown
public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override;
    virtual ULONG STDMETHODCALLTYPE AddRef(void) override;
    virtual ULONG STDMETHODCALLTYPE Release(void) override;

protected:
    tsm::UnknownImpl<ToneMediaSource> m_unknownImpl;
#pragma endregion
};
