#include "pch.h"

#include "ToneVideoStream.h"

/*static*/ bool ToneVideoStream::showInPane = false;

// Colors for channel number text and wave form of each channel.
// First color is used for channel 1, second for channel 2, and so on.
static const COLORREF colors[] = {
	RGB(0 ,0 ,0),			// Black
	RGB(0xff, 0x00, 0x00),	// Red
	RGB(0xff, 0xff, 0x00),	// Yellow
	RGB(0x00, 0xff, 0xff),	// Blue
	RGB(0x80, 0x00, 0x80),	// Violet
	RGB(0xff, 0xa5, 0x00),	// Orange
	RGB(0x00, 0x80, 0x00),	// Green
	RGB(0x00, 0x00, 0xff),	// Indigo
};

// Background color
static const COLORREF bgColor(RGB(0xc0, 0xc0, 0xc0));

static HRESULT initializeBitmapInfoHeader(IMFMediaType* mediaType, BITMAPINFOHEADER& bi);

ToneVideoStream::ToneVideoStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd, std::shared_ptr<IPcmData>& pcmData)
	: ToneMediaStream(mediaSource, sd)
	, m_pcmData(pcmData), m_startSampleIndex(0)
{
}


static const BITMAPINFOHEADER defaultBitmapInfoHeader = {
	sizeof(BITMAPINFOHEADER),
	480, 320,		// Width x Height(Bottom-Up DIB with the origin at the lower left corner.)
	1,
	24,				// BPP
	BI_RGB,
};

HRESULT ToneVideoStream::createStreamDescriptor(DWORD streamId, IMFStreamDescriptor** ppsd)
{
	CComPtr<IMFVideoMediaType> mediaType;
	auto& bi = defaultBitmapInfoHeader;
	HR_ASSERT_OK(MFCreateVideoMediaTypeFromBitMapInfoHeaderEx(&bi, bi.biSize, bi.biWidth, bi.biHeight, MFVideoInterlace_Progressive, 0, 1, 30, 0, &mediaType));

	IMFMediaType* mediaTypes[] = { mediaType };
	return ToneMediaStream::createStreamDescriptor(mediaTypes, streamId, ppsd);
}

HRESULT ToneVideoStream::onStart(const PROPVARIANT* pvarStartPosition)
{
	// Nothing to do.
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

HRESULT ToneVideoStream::onRequestSample(IMFSample* sample)
{
	// Sample duration in mSec.
	static const UINT32 duration = 33;		// 30 flames per second.

	BITMAPINFOHEADER bi;
	HR_ASSERT_OK(initializeBitmapInfoHeader(m_mediaType, bi));

	// Create memory DC and select color Bitmap.
	CDC dcSrc;
	dcSrc.CreateCompatibleDC(NULL);
	CBitmap hbitmapSrc;
	auto bpp = dcSrc.GetDeviceCaps(BITSPIXEL);	// Bits/Pixel of display device.
	hbitmapSrc.CreateBitmap(bi.biWidth, bi.biHeight, 1, bpp, NULL);
	dcSrc.SelectObject(hbitmapSrc);

	// Draw background and wave form on the background.
	drawBackground(dcSrc, bi.biWidth, bi.biHeight);
	drawWaveForm(dcSrc, bi.biWidth, bi.biHeight);

	// Create IMFMediaBuffer.
	CComPtr<IMFMediaBuffer> buffer;
	BYTE* rawBuffer;
	HR_ASSERT_OK(MFCreateMemoryBuffer(bi.biSizeImage, &buffer));
	HR_ASSERT_OK(buffer->Lock(&rawBuffer, nullptr, nullptr));

	// Copy contents of memory DC to the buffer.
	CDC dcDest;
	dcDest.CreateCompatibleDC(&dcSrc);
	LPVOID dibBuffer;
	auto hdib = CreateDIBSection(dcDest, (const BITMAPINFO*)&bi, DIB_RGB_COLORS, &dibBuffer, NULL, 0);
	HR_EXPECT(hdib && dibBuffer, E_UNEXPECTED);
	if(hdib) {
		dcDest.SelectObject(hdib);
		dcDest.BitBlt(0, 0, bi.biWidth, bi.biHeight, &dcSrc, 0, 0, SRCCOPY);
		CopyMemory(rawBuffer, dibBuffer, bi.biSizeImage);

		DeleteObject(hdib);
	}

	HR_ASSERT_OK(buffer->Unlock());
	HR_ASSERT_OK(buffer->SetCurrentLength(bi.biSizeImage));

	// Add the buffer to IMFSample object.
	sample->AddBuffer(buffer);

	LONGLONG sampleDuration = (LONGLONG)duration * 10000;	// 100-nanosecond units.
	sample->SetSampleDuration(sampleDuration);
	sample->SetSampleTime(m_sampleTime);
	m_sampleTime += sampleDuration;

	return S_OK;
}

void ToneVideoStream::drawBackground(CDC& dc, int width, int height)
{
	// Fill background.
	CRect bgRect(0, 0, width, height);
	CBrush bgBrush(bgColor);
	dc.FillRect(bgRect, &bgBrush);

	// Write text of each channel number using color as same as wave form.
	static const CString textFormat(_T("———— Channel %d"));
	auto textSize = dc.GetTextExtent(textFormat);
	auto channels = m_pcmData->getChannels();
	int margin = 2;
	int x = width - textSize.cx - margin;	// Right aligned.
	int y = margin;
	for(WORD ch = 0; ch < channels; ch++) {
		CString text;
		text.Format(textFormat, ch + 1);
		dc.SetTextColor(colors[ch % ARRAYSIZE(colors)]);
		dc.TextOut(x, y, text);
		y += (textSize.cy + margin);
	}
}

void ToneVideoStream::drawWaveForm(CDC& dc, int width, int height)
{
	auto sampleDataType = m_pcmData->getSampleDataType();
	auto channels = m_pcmData->getChannels();
	const auto& highValue = IPcmSample::getHighValue(sampleDataType);
	const auto& lowValue = IPcmSample::getLowValue(sampleDataType);
	const auto& zeroValue = IPcmSample::getZeroValue(sampleDataType);
	const auto valueHeight = highValue.getInt32() - lowValue.getInt32();

	if(!m_pcmSample || (m_pcmSample->getSampleCount() != m_pcmData->getSamplesPerCycle())) {
		// Copy 1-cycle samples to the buffer and create IPcmSample object that references the buffer.
		auto bufferSize = m_pcmData->getSampleBufferSize(0);
		m_sampleBuffer.reset(new BYTE[bufferSize]);
		m_pcmData->copyTo(m_sampleBuffer.get(), bufferSize);
		m_pcmSample.reset(createPcmSample(sampleDataType, m_sampleBuffer.get(), bufferSize));
		m_startSampleIndex = 0;
	}

	auto sampleCount = m_pcmSample->getSampleCount();
	size_t sampleIndex = m_startSampleIndex;
	for(LONG x = 0; x < width; x++) {
		for(WORD ch = 0; ch < channels; ch++) {
			auto value = (*m_pcmSample)[sampleIndex % sampleCount].getInt32() - zeroValue.getInt32();
			auto y = (int)((double)value * height / valueHeight);
			if(showInPane) {
				// Show wave form of each channel in it's pane.
				y /= channels;
				y += ((height / channels / 2) + (height / channels * ch));
			} else {
				// Show wave form of all channels in piles.
				y += (height / 2);
			}
			dc.SetPixel(x, y, colors[sampleIndex % channels % ARRAYSIZE(colors)]);
			sampleIndex++;
		}
	}

	m_startSampleIndex = (m_startSampleIndex + channels) % sampleCount;
}

HRESULT initializeBitmapInfoHeader(IMFMediaType* mediaType, BITMAPINFOHEADER& bi)
{
	GUID subType;
	mediaType->GetGUID(MF_MT_SUBTYPE, &subType);
	HR_ASSERT(subType == MFVideoFormat_RGB24, MF_E_INVALIDMEDIATYPE);

	bi = defaultBitmapInfoHeader;

	UINT32 width, height;
	HR_ASSERT_OK(MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &width, &height));
	bi.biWidth = width;
	bi.biHeight = height;
	UINT32 imageSize;
	HR_ASSERT_OK(MFCalculateImageSize(subType, width, height, &imageSize));
	HR_ASSERT(0 < imageSize, E_UNEXPECTED);
	bi.biSizeImage = imageSize;

	return S_OK;
}
