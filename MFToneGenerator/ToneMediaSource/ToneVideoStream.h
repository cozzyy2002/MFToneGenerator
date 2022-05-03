#pragma once

#include "ToneMediaStream.h"

class ToneVideoStream : public ToneMediaStream
{
public:
    ToneVideoStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd);

    static HRESULT createStreamDescriptor(DWORD streamId, IMFStreamDescriptor** ppsd);

protected:
    virtual HRESULT onStart(const PROPVARIANT* pvarStartPosition) override;
    virtual HRESULT onStop() override;
    virtual HRESULT onShutdown() override;
    virtual HRESULT onRequestSample(IMFSample* sample) override;

protected:
};
