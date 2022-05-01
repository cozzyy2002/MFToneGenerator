#pragma once

#include "PcmData/PcmData.h"
#include "MediaEventGenerator.h"
#include "Utils.h"

class ToneMediaSource;

/**
 * Base class of custom Media Streams.
 */
class ToneMediaStream : public IMFMediaStream, DoNotCopy
{
protected:
    ToneMediaStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd);

public:
    virtual ~ToneMediaStream() {}

    virtual HRESULT start(const PROPVARIANT* pvarStartPosition) = 0;
    virtual HRESULT stop() = 0;
    virtual HRESULT shutdown() = 0;

protected:
    ToneMediaSource* m_mediaSource;
    CComPtr<IMFStreamDescriptor> m_sd;

    template<DWORD Count>
    static HRESULT createStreamDescriptor(IMFMediaType* (&mediaTypes)[Count], IMFStreamDescriptor** ppsd) {
        HR_ASSERT(0 < Count, E_INVALIDARG);

        static DWORD streamId = 1;
        HR_ASSERT_OK(MFCreateStreamDescriptor(streamId++, Count, mediaTypes, ppsd));
        CComPtr<IMFMediaTypeHandler> mth;
        (*ppsd)->GetMediaTypeHandler(&mth);
        mth->SetCurrentMediaType(mediaTypes[0]);

        return S_OK;
    }

#pragma region Implementation of IMFMediaStream
public:
    virtual HRESULT STDMETHODCALLTYPE GetMediaSource(
        /* [out] */ __RPC__deref_out_opt IMFMediaSource** ppMediaSource) override;
    virtual HRESULT STDMETHODCALLTYPE GetStreamDescriptor(
        /* [out] */ __RPC__deref_out_opt IMFStreamDescriptor** ppStreamDescriptor) override;
    // Note: RequestSample() method should be implemented by derived class.

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
