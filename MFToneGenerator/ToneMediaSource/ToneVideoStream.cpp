#include "pch.h"

#include "ToneVideoStream.h"

/*static*/ bool ToneVideoStream::showInPane = false;

static const Pixel WhitePixel(0xff, 0xff, 0xff);
static const Pixel RedPixel(0xff, 0, 0);
static const Pixel GreenPixel(0, 0xff, 0);
static const Pixel BluePixel(0, 0, 0xff);
static const Pixel BlackPixel(0, 0, 0);

static const Pixel pixels[] = {BlackPixel, RedPixel, GreenPixel, BluePixel};

static HRESULT initializeBitmapInfoHeader(IMFMediaType* mediaType, BITMAPINFOHEADER& bi);

ToneVideoStream::ToneVideoStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd, std::shared_ptr<IPcmData>& pcmData)
	: ToneMediaStream(mediaSource, sd)
	, m_pcmData(pcmData), m_startSampleIndex(0)
{
}


static const BITMAPINFOHEADER bitmapInfoHeader = {
	sizeof(BITMAPINFOHEADER),
	480, 320,		// Width x Height(Bottom-Up DIB with the origin at the lower left corner.)
	1,
	Pixel::BitCount,	// BPP
	BI_RGB,
};

HRESULT ToneVideoStream::createStreamDescriptor(DWORD streamId, IMFStreamDescriptor** ppsd)
{
	CComPtr<IMFVideoMediaType> mediaType;
	auto& bi = bitmapInfoHeader;
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
	const LONG pixelCount = bi.biWidth * bi.biHeight;

	// Copy wave form bitmap into IMFMediaBuffer.
	CComPtr<IMFMediaBuffer> buffer;
	BYTE* rawBuffer;
	HR_ASSERT_OK(MFCreateMemoryBuffer(bi.biSizeImage, &buffer));
	HR_ASSERT_OK(buffer->Lock(&rawBuffer, nullptr, nullptr));

	// Draw background and wave form on the background.
	drawBackground(rawBuffer, bi);
	drawWaveForm(rawBuffer, bi);

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

class PixelArray
{
public:
	PixelArray(Pixel* buffer, UINT32 width, UINT32 height)
		: m_buffer(buffer), width(width), height(height) {}

	// Returns address of top pixel of the row.
	// Then the Pixel at row/column can be accessed by `PixelArray[row][column]` as 2-dimensional array.
	Pixel* operator[](UINT32 row) { return (&m_buffer[width * (row % height)]); }

	Pixel& at(UINT32 row, UINT32 col) { return (*this)[row][col % width]; }

	const UINT32 width;
	const UINT32 height;

protected:
	Pixel* m_buffer;
};

void ToneVideoStream::drawBackground(LPBYTE buffer, const BITMAPINFOHEADER& bi)
{
	// Create memory DC and select color Bitmap.
	CDC dcSrc;
	dcSrc.CreateCompatibleDC(NULL);
	CBitmap hbitmapSrc;
	auto bpp = dcSrc.GetDeviceCaps(BITSPIXEL);	// Bits/Pixel of display device.
	hbitmapSrc.CreateBitmap(bi.biWidth, bi.biHeight, 1, bpp, NULL);
	dcSrc.SelectObject(hbitmapSrc);

	// Fill background with white.
	dcSrc.PatBlt(0, 0, bi.biWidth, bi.biHeight, WHITENESS);

	// Write text of each channel number using color as same as wave form.
	CString textFormat(_T("———— Channel %d"));
	auto textSize = dcSrc.GetTextExtent(textFormat);
	auto channels = m_pcmData->getChannels();
	int margin = 2;
	int x = bi.biWidth - textSize.cx - margin;	// Right aligned.
	int y = margin;
	for(WORD ch = 0; ch < channels; ch++) {
		CString text;
		text.Format(textFormat, ch + 1);
		dcSrc.SetTextColor(pixels[ch % ARRAYSIZE(pixels)].getColorRef());
		dcSrc.TextOut(x, y, text);
		y += (textSize.cy + margin);
	}

	// Copy contents of memory DC to the buffer.
	CDC dcDest;
	dcDest.CreateCompatibleDC(&dcSrc);
	LPVOID dibBuffer;
	auto hdib = CreateDIBSection(dcDest, (const BITMAPINFO*)&bi, DIB_RGB_COLORS, &dibBuffer, NULL, 0);
	HR_EXPECT(hdib && dibBuffer, E_UNEXPECTED);
	if(hdib) {
		dcDest.SelectObject(hdib);
		dcDest.BitBlt(0, 0, bi.biWidth, bi.biHeight, &dcSrc, 0, 0, SRCCOPY);
		CopyMemory(buffer, dibBuffer, bi.biSizeImage);

		DeleteObject(hdib);
	}
}

void ToneVideoStream::drawWaveForm(LPBYTE buffer, const BITMAPINFOHEADER& bi)
{
	auto sampleDataType = m_pcmData->getSampleDataType();
	auto channels = m_pcmData->getChannels();
	const auto highValue = IPcmSample::getHighValue(sampleDataType);
	const auto lowValue = IPcmSample::getLowValue(sampleDataType);
	const auto valueHeight = highValue.getInt32() - lowValue.getInt32();

	if(!m_pcmSample || (m_pcmSample->getSampleCount() != m_pcmData->getSamplesPerCycle())) {
		// Copy 1-cycle samples to the buffer and create IPcmSample object that references the buffer.
		auto bufferSize = m_pcmData->getSampleBufferSize(0);
		m_sampleBuffer.reset(new BYTE[bufferSize]);
		m_pcmData->copyTo(m_sampleBuffer.get(), bufferSize);
		m_pcmSample.reset(createPcmSample(sampleDataType, m_sampleBuffer.get(), bufferSize));
		m_startSampleIndex = 0;
	}

	PixelArray pixelArray((Pixel*)buffer, bi.biWidth, bi.biHeight);
	auto sampleCount = m_pcmSample->getSampleCount();
	size_t sampleIndex = m_startSampleIndex;
	for(LONG i = 0; i < bi.biWidth; i++) {
		for(WORD ch = 0; ch < channels; ch++) {
			auto row = (*m_pcmSample)[sampleIndex % sampleCount].getInt32() * bi.biHeight / valueHeight;
			if(showInPane) {
				// Show wave form of each channel in it's pane.
				row /= channels;
				row += ((bi.biHeight / channels / 2) + (bi.biHeight / channels * (channels - 1 - ch)));
			} else {
				// Show wave form of all channels in piles.
				row += (bi.biHeight / 2);
			}
			auto col = i;
			pixelArray[row][col] = pixels[sampleIndex % channels % ARRAYSIZE(pixels)];
			sampleIndex++;
		}
	}

	m_startSampleIndex = (m_startSampleIndex + channels) % sampleCount;
}

HRESULT initializeBitmapInfoHeader(IMFMediaType* mediaType, BITMAPINFOHEADER& bi)
{
	GUID subType;
	mediaType->GetGUID(MF_MT_SUBTYPE, &subType);
	HR_ASSERT(subType == Pixel::VideoSubType, MF_E_INVALIDMEDIATYPE);

	ZeroMemory(&bi, sizeof(bi));
	bi.biSize = sizeof(bi);

	UINT32 width, height;
	HR_ASSERT_OK(MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &width, &height));
	bi.biWidth = width;
	bi.biHeight = height;
	bi.biPlanes = 1;
	bi.biBitCount = Pixel::BitCount;
	bi.biCompression = BI_RGB;
	UINT32 imageSize;
	HR_ASSERT_OK(MFCalculateImageSize(subType, width, height, &imageSize));
	HR_ASSERT(0 < imageSize, E_UNEXPECTED);
	bi.biSizeImage = imageSize;

	return S_OK;
}

/*static*/ REFGUID Pixel::VideoSubType = (sizeof(Pixel) == 3) ? MFVideoFormat_RGB24 : MFVideoFormat_RGB32;
/*static*/ const WORD Pixel::BitCount = sizeof(Pixel) * 8;
