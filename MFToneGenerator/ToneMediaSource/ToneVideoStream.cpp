﻿#include "pch.h"

#include "ToneVideoStream.h"

/*static*/ bool ToneVideoStream::showInPane = false;

// Colors for channel number text and wave form of each channel.
// First color is used for channel 1, second for channel 2, and so on.
static const D2D1::ColorF colors[] = {
	D2D1::ColorF::Black,
	D2D1::ColorF::Red,
	D2D1::ColorF::Yellow,
	D2D1::ColorF::Blue,
	D2D1::ColorF::Violet,
	D2D1::ColorF::Orange,
	D2D1::ColorF::Green,
	D2D1::ColorF::Indigo,
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
	1,				// Planes
	32,				// BPP
	BI_RGB,
};

HRESULT ToneVideoStream::createStreamDescriptor(DWORD streamId, IMFStreamDescriptor** ppsd)
{
	CComPtr<IMFVideoMediaType> mediaType;
	auto& bi = defaultBitmapInfoHeader;
	HR_ASSERT_OK(MFCreateVideoMediaTypeFromBitMapInfoHeaderEx(&bi, bi.biSize, bi.biWidth, bi.biHeight, MFVideoInterlace_Progressive, 0, 1, 30, 0, &mediaType));

	UINT32 stride;
	auto hr = HR_EXPECT_OK(mediaType->GetUINT32(MF_MT_DEFAULT_STRIDE, &stride));
	if(SUCCEEDED(hr)) {
		auto iStride = (INT32)stride;
		if(iStride < 0) {
			// Set positive value as default stride to tell that the image is top-down.
			HR_ASSERT_OK(mediaType->SetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32)(-iStride)));
		}
	}

	IMFMediaType* mediaTypes[] = { mediaType };
	return ToneMediaStream::createStreamDescriptor(mediaTypes, streamId, ppsd);
}

HRESULT ToneVideoStream::onStart(const PROPVARIANT* pvarStartPosition)
{
	HR_ASSERT_OK(createDeviceResources());

	CComPtr<IDWriteFactory> dwFactory;
	HR_ASSERT_OK(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&dwFactory)));

	HR_ASSERT_OK(dwFactory->CreateTextFormat(
		L"System", NULL,
		DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		20, L"", &m_textFormat));
	HR_ASSERT_OK(m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING));

	return S_OK;
}

HRESULT ToneVideoStream::onStop()
{
	HR_ASSERT_OK(discardDeviceResources());

	return S_OK;
}

HRESULT ToneVideoStream::onShutdown()
{
	// Nothing to do.
	return S_OK;
}

HRESULT ToneVideoStream::createDeviceResources()
{
	BITMAPINFOHEADER bi;
	HR_ASSERT_OK(initializeBitmapInfoHeader(m_mediaType, bi));

	CComPtr<IWICImagingFactory> wicFactory;
	HR_ASSERT_OK(wicFactory.CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER));

	// Note: GUID_WICPixelFormat32bppBGR is suitable for MFVideoFormat_RGB32
	HR_ASSERT_OK(wicFactory->CreateBitmap(bi.biWidth, bi.biHeight, GUID_WICPixelFormat32bppBGR, WICBitmapCacheOnDemand, &m_bitmap));
	CComPtr<ID2D1Factory> d2d1Factory;
	HR_ASSERT_OK(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &d2d1Factory));
	auto prop = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat());
	HR_ASSERT_OK(d2d1Factory->CreateWicBitmapRenderTarget(m_bitmap, prop, &m_renderTarget));

	m_brushes.reserve(ARRAYSIZE(colors));
	for(auto& color : colors) {
		CComPtr<ID2D1SolidColorBrush> brush;
		HR_ASSERT_OK(m_renderTarget->CreateSolidColorBrush(color, &brush));
		m_brushes.push_back(brush);
	}

	return S_OK;
}

HRESULT ToneVideoStream::discardDeviceResources()
{
	m_brushes.clear();
	m_renderTarget.Release();
	m_bitmap.Release();

	return S_OK;
}

HRESULT ToneVideoStream::onRequestSample(IMFSample* sample)
{
	if(!m_renderTarget) { return S_FALSE; }

	// Sample duration in mSec.
	static const UINT32 duration = 33;		// 30 flames per second.

	BITMAPINFOHEADER bi;
	HR_ASSERT_OK(initializeBitmapInfoHeader(m_mediaType, bi));

	// Draw background and wave form to IWICBitmap through ID2D1RenderTarget.
	m_renderTarget->BeginDraw();
	auto width = (float)bi.biWidth;
	auto height = (float)bi.biHeight;
	drawBackground(width, height);
	drawWaveForm(width, height);
	// Workaround to avoid that CComPtr::operator->() fails at `ATLASSERT(p!=NULL)`.
	if(!m_renderTarget) { return S_FALSE; }
	D2D1_TAG tag1, tag2;
	auto hr = HR_EXPECT_OK(m_renderTarget->EndDraw(&tag1, &tag2));
	if(FAILED(hr)) {
		Logger logger;
		if(hr == D2DERR_RECREATE_TARGET) {
			logger.log(_T("Recreate ID2D1RenderTarget"));
			HR_ASSERT_OK(discardDeviceResources());
			HR_ASSERT_OK(createDeviceResources());
		} else {
			logger.log(_T("ID2D1RenderTarget::EndDraw() failed. HRESULT=0x%p, TAG=%I64u:%I64u"), hr, tag1, tag2);
			return hr;
		}
	}

	// Create IMFMediaBuffer and copy contents of IWICBitmap to it.
	WICRect wicRect = {0, 0, bi.biWidth, bi.biHeight};
	CComPtr<IMFMediaBuffer> buffer;
	UINT bitmapSize;
	{
		// In this scope:
		//   IWICBitmapLock object is retrieved from IWICBitmap and released.
		//   IMFMediaBuffer::Lock() and Unlock() are called.
		//   Copy contents of IWICBitmap to IMFMediaBuffer.
		CComPtr<IWICBitmapLock> bitmapLock;
		HR_ASSERT_OK(m_bitmap->Lock(&wicRect, WICBitmapLockRead, &bitmapLock));
		BYTE* bitmap;
		HR_ASSERT_OK(bitmapLock->GetDataPointer(&bitmapSize, &bitmap));

		BYTE* rawBuffer;
		HR_ASSERT_OK(MFCreateMemoryBuffer(bitmapSize, &buffer));
		HR_ASSERT_OK(buffer->Lock(&rawBuffer, nullptr, nullptr));
		ScopedDeleter<IMFMediaBuffer> unlockBuffer(buffer, [](IMFMediaBuffer* p) { HR_EXPECT_OK(p->Unlock()); });

		CopyMemory(rawBuffer, bitmap, bitmapSize);
	}
	HR_ASSERT_OK(buffer->SetCurrentLength(bitmapSize));

	// Add the buffer to IMFSample object.
	sample->AddBuffer(buffer);

	LONGLONG sampleDuration = (LONGLONG)duration * 10000;	// 100-nanosecond units.
	sample->SetSampleDuration(sampleDuration);
	sample->SetSampleTime(m_sampleTime);
	m_sampleTime += sampleDuration;

	return S_OK;
}

void ToneVideoStream::drawBackground(float width, float height)
{
	// Fill background.
	auto bgColor = D2D1::ColorF(D2D1::ColorF::WhiteSmoke);
	m_renderTarget->Clear(bgColor);

	// Write text of each channel number using color as same as wave form.
	static const CString textFormat(_T("———— Channel %d"));
	D2D1_SIZE_F textSize = {100, 20};		// TODO: Retrieve text size.
	auto channels = m_pcmData->getChannels();
	static const float margin = 2;
	textSize.height += margin;
	D2D1_RECT_F textRect = {margin, margin, width - margin, textSize.height};
	for(WORD ch = 0; ch < channels; ch++) {
		CString text;
		text.Format(textFormat, ch + 1);
		auto& textBrush = m_brushes[ch % m_brushes.size()];
		m_renderTarget->DrawText(text, text.GetLength(), m_textFormat, textRect, textBrush);
		textRect.top += textSize.height;
		textRect.bottom += textSize.height;
	}
}

void ToneVideoStream::drawWaveForm(float width, float height)
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
			//dc.SetPixel(x, y, colors[sampleIndex % channels % ARRAYSIZE(colors)]);
			sampleIndex++;
		}
	}

	m_startSampleIndex = (m_startSampleIndex + channels) % sampleCount;
}

HRESULT initializeBitmapInfoHeader(IMFMediaType* mediaType, BITMAPINFOHEADER& bi)
{
	GUID subType;
	mediaType->GetGUID(MF_MT_SUBTYPE, &subType);
	HR_ASSERT(subType == MFVideoFormat_RGB32, MF_E_INVALIDMEDIATYPE);

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
