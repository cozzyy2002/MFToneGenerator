#include "pch.h"

#include "ToneMediaSource.h"
#include "ToneAudioStream.h"

ToneMediaSource::ToneMediaSource(std::shared_ptr<IPcmData>& pcmData)
	: m_pcmData(pcmData), m_unknownImpl(this)
{
}

HRESULT ToneMediaSource::checkShutdown()
{
	HR_ASSERT_OK(m_eventGenerator.checkShutdown());

	return S_OK;
}

#pragma region Implementation of IMFMediaSource
HRESULT __stdcall ToneMediaSource::GetCharacteristics(__RPC__out DWORD* pdwCharacteristics)
{
	HR_ASSERT(pdwCharacteristics, E_POINTER);
	HR_ASSERT_OK(checkShutdown());

	*pdwCharacteristics = MFMEDIASOURCE_DOES_NOT_USE_NETWORK;
	return S_OK;
}

HRESULT __stdcall ToneMediaSource::CreatePresentationDescriptor(_Outptr_ IMFPresentationDescriptor** ppPresentationDescriptor)
{
	HR_ASSERT(ppPresentationDescriptor, E_POINTER);
	HR_ASSERT_OK(checkShutdown());

	if(!m_pd) {
		// Create Mediatype and StreamDesctiptor for ToneMediaStream(Audio stream)
		CComPtr<IMFStreamDescriptor> sdAudio;
		HR_ASSERT_OK(ToneAudioStream::createStreamDescriptor(m_pcmData.get(), (DWORD)StreamId::ToneAudio, &sdAudio));

		// Create PresentationDesctiptor and select all streams.
		IMFStreamDescriptor* sds[] = { sdAudio.p };
		HR_ASSERT_OK(MFCreatePresentationDescriptor(ARRAYSIZE(sds), sds, &m_pd));
		DWORD sdCount;
		HR_ASSERT_OK(m_pd->GetStreamDescriptorCount(&sdCount));
		for (DWORD i = 0; i < sdCount; i++) {
			m_pd->SelectStream(i);
		}
	}

	return HR_EXPECT_OK(m_pd->Clone(ppPresentationDescriptor));
}

HRESULT __stdcall ToneMediaSource::Start(__RPC__in_opt IMFPresentationDescriptor* pPresentationDescriptor, __RPC__in_opt const GUID* pguidTimeFormat, __RPC__in_opt const PROPVARIANT* pvarStartPosition)
{
	DWORD sdCount;
	m_pd->GetStreamDescriptorCount(&sdCount);
	for (DWORD isd = 0; isd < sdCount; isd++) {
		CComPtr<IMFStreamDescriptor> sd;
		BOOL selected;
		HR_ASSERT_OK(pPresentationDescriptor->GetStreamDescriptorByIndex(isd, &selected, &sd));
		if(!selected) continue;

		CComPtr<ToneMediaStream> stream;
		DWORD dwStreamId;
		sd->GetStreamIdentifier(&dwStreamId);
		auto streamId = (StreamId)dwStreamId;
		auto it = m_mediaStreams.find(streamId);
		auto isNewStream = (it == m_mediaStreams.end());
		MediaEventType met = isNewStream ? MENewStream : MEUpdatedStream;
		if (isNewStream) {
			switch (streamId) {
			case StreamId::ToneAudio:
				stream = new ToneAudioStream(this, sd, m_pcmData);
				m_mediaStreams[streamId] = stream;
				break;
			default:
				// Unknonw Stream ID
				return E_UNEXPECTED;
			}
		} else {
			stream = it->second;
		}
		PROPVARIANT value = { VT_UNKNOWN };
		value.punkVal = stream;
		m_eventGenerator.QueueEvent(met, &value);
		HR_ASSERT_OK(stream->start(pvarStartPosition));
	}

	m_eventGenerator.QueueEvent(MESourceStarted, pvarStartPosition);

	return S_OK;
}

HRESULT __stdcall ToneMediaSource::Stop(void)
{
	for(auto& pair : m_mediaStreams) {
		pair.second->stop();
	}

	m_eventGenerator.QueueEvent(MESourceStopped);

	return S_OK;
}

HRESULT __stdcall ToneMediaSource::Pause(void)
{
	return E_NOTIMPL;
}

HRESULT __stdcall ToneMediaSource::Shutdown(void)
{
	for (auto& pair : m_mediaStreams) {
		pair.second->shutdown();
	}
	m_mediaStreams.clear();

	m_eventGenerator.shutdown();

	return S_OK;
}

#pragma region Implementation of IMFMediaEventGenerator

HRESULT __stdcall ToneMediaSource::GetEvent(DWORD dwFlags, __RPC__deref_out_opt IMFMediaEvent** ppEvent)
{
	return m_eventGenerator.GetEvent(dwFlags, ppEvent);
}
HRESULT __stdcall ToneMediaSource::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	return m_eventGenerator.BeginGetEvent(pCallback, punkState);
}
HRESULT __stdcall ToneMediaSource::EndGetEvent(IMFAsyncResult* pResult, _Out_  IMFMediaEvent** ppEvent)
{
	return m_eventGenerator.EndGetEvent(pResult, ppEvent);
}
HRESULT __stdcall ToneMediaSource::QueueEvent(MediaEventType met, __RPC__in REFGUID guidExtendedType, HRESULT hrStatus, __RPC__in_opt const PROPVARIANT* pvValue)
{
	return m_eventGenerator.QueueEvent(met, guidExtendedType, hrStatus, pvValue);
}

#pragma endregion

#pragma region Implementation of IUnknown

HRESULT __stdcall ToneMediaSource::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject)
{
	HR_ASSERT(ppvObject, E_POINTER);

	static const QITAB qitab[] = {
		QITABENT(ToneMediaSource, IMFMediaSource),
		QITABENT(ToneMediaSource, IMFMediaEventGenerator),
		{ 0 }
	};
	return m_unknownImpl.QueryInterface(riid, ppvObject, qitab);
}

ULONG __stdcall ToneMediaSource::AddRef(void)
{
	return m_unknownImpl.AddRef();
}

ULONG __stdcall ToneMediaSource::Release(void)
{
	return m_unknownImpl.Release();
}

#pragma endregion
