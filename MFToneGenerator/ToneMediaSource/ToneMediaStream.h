#pragma once

#include "PcmData/PcmData.h"
#include "MediaEventGenerator.h"
#include "Utils.h"

class ToneMediaSource;

/**
 * Custom Media Stream generates Tone Wave.
 */
class ToneMediaStream : public IMFMediaStream, DoNotCopy
{
public:
    ToneMediaStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd, std::shared_ptr<IPcmData>& pcmData);

    HRESULT start();
    HRESULT stop();
    HRESULT shutdown();
    HRESULT QueueEvent(MediaEventType met, const PROPVARIANT* pvValue = nullptr) { return m_eventGenerator.QueueEvent(met, pvValue); }

protected:
    ToneMediaSource* m_mediaSource;
    CComPtr<IMFStreamDescriptor> m_sd;
    float m_key;

    // PCM data generator.
    std::shared_ptr<IPcmData> m_pcmData;

#pragma region Implementation of IMFMediaStream
public:
    virtual HRESULT STDMETHODCALLTYPE GetMediaSource(
        /* [out] */ __RPC__deref_out_opt IMFMediaSource** ppMediaSource) override;
    virtual HRESULT STDMETHODCALLTYPE GetStreamDescriptor(
        /* [out] */ __RPC__deref_out_opt IMFStreamDescriptor** ppStreamDescriptor) override;
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE RequestSample(
        /* [in] */ IUnknown* pToken) override;

protected:
    LONGLONG m_sampleTime;
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
    tsm::UnknownImpl<ToneMediaStream> m_unknownImpl;
#pragma endregion
};
