#pragma once

#include "Utils.h"

class Event;
class State;

class Context : public tsm::AsyncContext<Event, State>
{
public:
	Context(HWND hWnd, UINT msg);
	~Context();

	void setAudioFileName(LPCTSTR value) { m_audioFileName = value; }
	LPCTSTR getAudioFileName() const { return m_audioFileName.c_str(); }

	HRESULT setup();
	HRESULT setKey(Event* event);
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

	CComPtr<MediaSessionCallback> m_callback;

	class StateMonitor : public tsm::StateMonitor<Context, Event, State>, public Logger
	{
	public:
		virtual void onEventTriggered(Context* context, Event* event) override;
		virtual void onStateChanged(Context* context, Event* event, State* previous, State* next) override;
	};

	std::unique_ptr<StateMonitor> m_stateMonitor;
};
