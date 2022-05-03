#include "pch.h"

#include "ToneVideoStream.h"

ToneVideoStream::ToneVideoStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd)
	: ToneMediaStream(mediaSource, sd)
{
}

HRESULT ToneVideoStream::createStreamDescriptor(DWORD streamId, IMFStreamDescriptor** ppsd)
{
	CComPtr<IMFMediaType> mediatype;
	MFCreateMediaType(&mediatype);
	return S_OK;
}

HRESULT ToneVideoStream::onStart(const PROPVARIANT* pvarStartPosition)
{
	return E_NOTIMPL;
}

HRESULT ToneVideoStream::onStop()
{
	return E_NOTIMPL;
}

HRESULT ToneVideoStream::onShutdown()
{
	return E_NOTIMPL;
}

HRESULT ToneVideoStream::onRequestSample(IMFSample* sample)
{
	return E_NOTIMPL;
}
