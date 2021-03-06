#pragma once

#include "ToneMediaStream.h"
#include <PcmData/PcmSample.h>

class ToneVideoStream : public ToneMediaStream
{
public:
    ToneVideoStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd, std::shared_ptr<IPcmData>& pcmData);

    static HRESULT createStreamDescriptor(DWORD streamId, IMFStreamDescriptor** ppsd);
    static bool showInPane;

protected:
    virtual HRESULT onStart(const PROPVARIANT* pvarStartPosition) override;
    virtual HRESULT onStop() override;
    virtual HRESULT onShutdown() override;
    virtual HRESULT onRequestSample(IMFSample* sample) override;

protected:
    std::shared_ptr<IPcmData> m_pcmData;
    std::unique_ptr<IPcmSample> m_pcmSample;
    std::unique_ptr<BYTE[]> m_sampleBuffer;

    // Sample index of m_pcmSample to draw wave form at first column of pixel.
    // This value is updated in evry invoking drawWaveForm() method
    // so that wave form moves from right to left.
    size_t m_startSampleIndex;

    void drawBackground(CDC& dc, int width, int height);
    void drawWaveForm(CDC& dc, int width, int height);
};
