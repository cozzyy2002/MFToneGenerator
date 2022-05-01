#pragma once

#include "ToneMediaStream.h"

class ToneAudioStream : public ToneMediaStream
{
public:
    ToneAudioStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd, std::shared_ptr<IPcmData>& pcmData);

    virtual HRESULT start(const PROPVARIANT* pvarStartPosition) override;
    virtual HRESULT stop() override;
    virtual HRESULT shutdown() override;

    static HRESULT createStreamDescriptor(IPcmData* pPcmData, DWORD streamId, IMFStreamDescriptor** ppsd);

#pragma region Implementation of IMFMediaStream
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE RequestSample(
        /* [in] */ IUnknown* pToken) override;
#pragma endregion

protected:
    float m_key;

    // PCM data generator.
    std::shared_ptr<IPcmData> m_pcmData;
};
