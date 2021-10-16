#pragma once

#include "IContext.h"
#include "ToneMediaSource/PcmData.h"
#include "Utils.h"
#include <functional>

namespace statemachine {

class Event;
class State;

class Context : public IContext, public tsm::AsyncContext<Event, State>, public Logger
{
public:
	Context(HWND hWnd, UINT msg);
	~Context();

	void callback(std::function<void(ICallback*)> func);
	IMFMediaSession* getSession() { return m_session; }

#pragma region Implementation of IContext
	virtual void setCallback(ICallback* callback) override { m_callback = callback; }
	virtual ICallback* getCallback() override { return m_callback; }

	virtual HRESULT setup() override;
	virtual HRESULT shutdown() override;
	virtual HRESULT startTone(IPcmData* pcmData) override;
	virtual HRESULT startFile(LPCTSTR fileName) override;
	virtual HRESULT stop() override;
	virtual HRESULT pauseResume() override;
#pragma endregion

	void setPcmData(IPcmData* pcmData)
	{
		m_pcmData = pcmData;
		m_audioFileName.clear();
	}
	void setAudioFileName(LPCTSTR fileName)
	{
		m_pcmData.Release();
		m_audioFileName = fileName;
	}
	HRESULT setupSession();
	HRESULT startSession();
	HRESULT stopSession();
	HRESULT closeSession();
	HRESULT shutdownSession();
	HRESULT pauseSession();
	HRESULT resumeSession();

	virtual tsm::IStateMonitor* _getStateMonitor() override { return m_stateMonitor.get(); }

protected:
	using BaseClass = tsm::AsyncContext<Event, State>;
	ICallback* m_callback;
	CComPtr<IPcmData> m_pcmData;
	std::tstring m_audioFileName;
	CComPtr<IMFMediaSource> m_source;
	CComPtr<IMFMediaSession> m_session;

	class MediaSessionCallback : public IMFAsyncCallback, public Logger
	{
	public:
		MediaSessionCallback(Context* context, IMFMediaSession* session) : m_context(context), m_session(session), m_unknownImpl(this) {}

		HRESULT beginGetEvent();

#pragma region Implementation of IMFAsyncCallback
		virtual HRESULT STDMETHODCALLTYPE GetParameters(
			/* [out] */ __RPC__out DWORD* pdwFlags,
			/* [out] */ __RPC__out DWORD* pdwQueue) override {
			return E_NOTIMPL;
		}
		virtual HRESULT STDMETHODCALLTYPE Invoke(
			/* [in] */ __RPC__in_opt IMFAsyncResult* pAsyncResult) override;
#pragma endregion

#pragma region Implementation of IUnknown
		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
			static const QITAB qitab[] = {
				QITABENT(MediaSessionCallback, IMFAsyncCallback),
				{ 0 }
			};
			return m_unknownImpl.QueryInterface(riid, ppvObject, qitab);
		}
		virtual ULONG STDMETHODCALLTYPE AddRef(void) override { return m_unknownImpl.AddRef(); }
		virtual ULONG STDMETHODCALLTYPE Release(void) override { return m_unknownImpl.Release(); }
#pragma endregion

	protected:
		Context* m_context;
		IMFMediaSession* m_session;
		tsm::UnknownImpl<MediaSessionCallback> m_unknownImpl;
	};

	CComPtr<MediaSessionCallback> m_mediaSessionCallback;

	class StateMonitor : public tsm::StateMonitor<Context, Event, State>, public Logger
	{
	public:
		virtual void onEventTriggered(Context* context, Event* event) override;
		virtual void onStateChanged(Context* context, Event* event, State* previous, State* next) override;
	};

	std::unique_ptr<StateMonitor> m_stateMonitor;
};

}
