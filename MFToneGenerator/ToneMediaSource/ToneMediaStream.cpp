#include "pch.h"

#include "ToneMediaStream.h"
#include "ToneMediaSource.h"

#include <thread>

ToneMediaStream::ToneMediaStream(ToneMediaSource* mediaSource, IMFStreamDescriptor* sd)
	: m_mediaSource(mediaSource), m_sd(sd), m_sampleTime(0), m_unknownImpl(this)
{
}

HRESULT __stdcall ToneMediaStream::RequestSample(IUnknown* pToken)
{
	HR_ASSERT_OK(m_eventGenerator.checkShutdown());
	HR_ASSERT(m_mediaSource->isStarted(), MF_E_MEDIA_SOURCE_WRONGSTATE);

	{
		// Add the token to end of the queue.
		CriticalSection lock(m_tokensLock);
		m_tokens.push_back(pToken);
	}

	// Create IMFSample to be passed to onRequestSample() method of derived class.
	CComPtr<IMFSample> sample;
	HR_ASSERT_OK(MFCreateSample(&sample));

	// Call onRequestSample() method of derived class on the worker thread.
	std::thread workerthread([this](CComPtr<IMFSample> sample)
		{
			if(FAILED(HR_EXPECT(m_mediaSource->isStarted(), MF_E_MEDIA_SOURCE_WRONGSTATE))) { return; }
			if(FAILED(HR_EXPECT_OK(m_eventGenerator.checkShutdown()))) { return; }

			CComPtr<IUnknown> token;
			{
				CriticalSection lock(m_tokensLock);
				if(!m_tokens.empty()) {
					// Retrieve a token from top of the queue.
					token = m_tokens.front();
					m_tokens.pop_front();
				} else {
					// No token exists in the queue.
					// Created sample is disdarded.
					return;
				}
			}

			auto hr = HR_EXPECT_OK(onRequestSample(sample));
			if(hr == S_OK) {
				// Ready to deliver sample.
				// Send MEMediaSample Event with the sample as Event Value.
				if(token) {
					sample->SetUnknown(MFSampleExtension_Token, token);
				}
				PROPVARIANT value = { VT_UNKNOWN };
				value.punkVal = sample;
				m_eventGenerator.QueueEvent(MEMediaSample, &value);
			} else if(FAILED(hr)) {
				// Error has occurred.
				m_eventGenerator.QueueEvent(MEError, GUID_NULL, hr, nullptr);
			}
			// NOTE: In case hr == S_FALSE, do nothing.
		},
		sample);

	workerthread.detach();

	return S_OK;
}

HRESULT ToneMediaStream::start(const PROPVARIANT* pvarStartPosition)
{
	CComPtr<IMFMediaTypeHandler> mth;
	HR_ASSERT_OK(m_sd->GetMediaTypeHandler(&mth));
	m_mediaType.Release();
	HR_ASSERT_OK(mth->GetCurrentMediaType(&m_mediaType));
	HR_ASSERT_OK(onStart(pvarStartPosition));

	m_eventGenerator.QueueEvent(MEStreamStarted, pvarStartPosition);

	return S_OK;
}

HRESULT ToneMediaStream::stop()
{
	HR_ASSERT_OK(onStop());

	m_tokens.clear();
	m_eventGenerator.QueueEvent(MEStreamStopped);

	return S_OK;
}

HRESULT ToneMediaStream::shutdown()
{
	HR_EXPECT_OK(onShutdown());

	{
		// Add the token to end of the queue.
		CriticalSection lock(m_tokensLock);
		m_tokens.clear();
	}

	m_eventGenerator.shutdown();

	return S_OK;
}

HRESULT __stdcall ToneMediaStream::GetMediaSource(__RPC__deref_out_opt IMFMediaSource** ppMediaSource)
{
	HR_ASSERT(ppMediaSource, E_POINTER);
	HR_ASSERT_OK(m_eventGenerator.checkShutdown());

	*ppMediaSource = m_mediaSource;
	m_mediaSource->AddRef();
	return S_OK;
}

HRESULT __stdcall ToneMediaStream::GetStreamDescriptor(__RPC__deref_out_opt IMFStreamDescriptor** ppStreamDescriptor)
{
	HR_ASSERT_OK(m_eventGenerator.checkShutdown());

	return HR_EXPECT_OK(m_sd.CopyTo(ppStreamDescriptor));
}

#pragma region Implementation of IMFMediaEventGenerator

HRESULT __stdcall ToneMediaStream::GetEvent(DWORD dwFlags, __RPC__deref_out_opt IMFMediaEvent** ppEvent)
{
	return m_eventGenerator.GetEvent(dwFlags, ppEvent);
}
HRESULT __stdcall ToneMediaStream::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	return m_eventGenerator.BeginGetEvent(pCallback, punkState);
}
HRESULT __stdcall ToneMediaStream::EndGetEvent(IMFAsyncResult* pResult, _Out_  IMFMediaEvent** ppEvent)
{
	return m_eventGenerator.EndGetEvent(pResult, ppEvent);
}
HRESULT __stdcall ToneMediaStream::QueueEvent(MediaEventType met, __RPC__in REFGUID guidExtendedType, HRESULT hrStatus, __RPC__in_opt const PROPVARIANT* pvValue)
{
	return m_eventGenerator.QueueEvent(met, guidExtendedType, hrStatus, pvValue);
}

#pragma endregion

#pragma region Implementation of IUnknown

HRESULT __stdcall ToneMediaStream::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject)
{
	HR_ASSERT(ppvObject, E_POINTER);

	static const QITAB qitab[] = {
		QITABENT(ToneMediaStream, IMFMediaStream),
		QITABENT(ToneMediaStream, IMFMediaEventGenerator),
		{ 0 }
	};
	return m_unknownImpl.QueryInterface(riid, ppvObject, qitab);
}

ULONG __stdcall ToneMediaStream::AddRef(void)
{
	return m_unknownImpl.AddRef();
}

ULONG __stdcall ToneMediaStream::Release(void)
{
	return m_unknownImpl.Release();
}

#pragma endregion
