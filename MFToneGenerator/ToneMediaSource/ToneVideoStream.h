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
    std::unique_ptr<BYTE[]> m_background;

    // Sample index of m_pcmSample to draw wave form at first column of pixel.
    // This value is updated in evry invoking drawWaveForm() method
    // so that wave form moves from right to left.
    size_t m_startSampleIndex;

    void drawWaveForm(LPBYTE buffer, const BITMAPINFOHEADER& bi);
};

struct Pixel
{
    Pixel() : data{ 0xff, 0xff, 0xff } {}
    Pixel(BYTE R, BYTE G, BYTE B) : data{ B, G, R } {}

    BYTE data[3];
    BYTE& R() { return data[2]; }
    BYTE& G() { return data[1]; }
    BYTE& B() { return data[0]; }
};
