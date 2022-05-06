#include "pch.h"

#include "ToneVideoStream.h"

ToneVideoStream::ToneVideoStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd)
	: ToneMediaStream(mediaSource, sd)
{
}


static const BITMAPINFOHEADER bitmapInfoHeader = {
	sizeof(BITMAPINFOHEADER),
	480, 320,		// Width x Height(Bottom-Up DIB with the origin at the lower left corner.)
	1,
	24,				// BPP
	BI_RGB,
};

HRESULT ToneVideoStream::createStreamDescriptor(DWORD streamId, IMFStreamDescriptor** ppsd)
{
	CComPtr<IMFVideoMediaType> mediaType;
	auto& bi = bitmapInfoHeader;
	MFCreateVideoMediaTypeFromBitMapInfoHeaderEx(&bi, bi.biSize, bi.biWidth, bi.biHeight, MFVideoInterlace_Progressive, 0, 1, 30, 0, &mediaType);

	IMFMediaType* mediaTypes[] = { mediaType };
	return ToneMediaStream::createStreamDescriptor(mediaTypes, streamId, ppsd);
}

HRESULT ToneVideoStream::onStart(const PROPVARIANT* pvarStartPosition)
{
	return S_OK;
}

HRESULT ToneVideoStream::onStop()
{
	// Nothing to do.
	return S_OK;
}

HRESULT ToneVideoStream::onShutdown()
{
	// Nothing to do.
	return S_OK;
}

struct Pixel
{
	Pixel() : data{ 0xff, 0xff, 0xff } {}
	Pixel(BYTE R, BYTE G, BYTE B) : data{ B, G, R } {}

	BYTE data[3];
	BYTE& R() { return data[2]; }
	BYTE& G() { return data[1]; }
	BYTE& B() { return data[0]; }
};

static const Pixel WhitePixel(0xff, 0xff, 0xff);
static const Pixel RedPixel(0xff, 0, 0);
static const Pixel GreenPixel(0, 0xff, 0);
static const Pixel BluePixel(0, 0, 0xff);

static const Pixel bgs[] = { WhitePixel, /*RedPixel,*/ GreenPixel, BluePixel };

HRESULT ToneVideoStream::onRequestSample(IMFSample* sample)
{
	// Sample duration in mSec.
	static const UINT32 duration = 33;		// 30 flames per second.

	auto& bi = bitmapInfoHeader;
	const LONG pixelCount = bi.biWidth * bi.biHeight;
	const LONG bufferSize = pixelCount * sizeof(Pixel);

	// Generate wave form bitmap.
	CComPtr<IMFMediaBuffer> buffer;
	BYTE* rawBuffer;
	HR_ASSERT_OK(MFCreateMemoryBuffer(bufferSize, &buffer));
	HR_ASSERT_OK(buffer->Lock(&rawBuffer, nullptr, nullptr));

	static WORD count = 0;
	WORD index = (count++ / 30) % ARRAYSIZE(bgs);

	auto pixels = (Pixel*)rawBuffer;
	for (LONG i = 0; i < pixelCount; i++) { pixels[i] = WhitePixel; }
	LONG start = bi.biWidth * (bi.biHeight / 2);
	for (LONG i = start; i < (start + (bi.biWidth * 2)); i++) { pixels[i] = RedPixel; }

	HR_ASSERT_OK(buffer->Unlock());
	HR_ASSERT_OK(buffer->SetCurrentLength(bufferSize));

	// Add the buffer to IMFSample object.
	sample->AddBuffer(buffer);

	LONGLONG sampleDuration = (LONGLONG)duration * 10000;	// 100-nanosecond units.
	sample->SetSampleDuration(sampleDuration);
	sample->SetSampleTime(m_sampleTime);
	m_sampleTime += sampleDuration;

	return S_OK;
}
