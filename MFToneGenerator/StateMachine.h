#pragma once

#include "Utils.h"

class Context;
class Event;
class State;

class Context : public tsm::AsyncContext<Event, State>
{
public:
	Context(HWND hWnd, UINT msg);
	~Context();

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
			/* [out] */ __RPC__out DWORD* pdwQueue) override { return E_NOTIMPL;}
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

class Event : public tsm::Event<Context>, public StringFormatter
{
public:
	enum class Type
	{
		Unknown,

		// Control Events from UI.
		StartStop,
		PauseResume,
		SetKey,

		// Media Session Events.
		MEEndOfPresentation,
		MEError,
		MESessionClosed,
		MESessionEnded,
		MESessionNotifyPresentationTime,
		MESessionStarted,
		MESessionPaused,
		MESessionStopped,
		MESessionTopologySet,
		MESessionTopologyStatus,
		MESessionCapabilitiesChanged,

		MAX
	};

	Event(Type type) : type(type) {}
	const Type type;

	virtual std::tstring toString() const override;

protected:
	mutable std::tstring m_string;
};

class SetKeyEvent : public Event
{
public:
	SetKeyEvent(float key) : Event(Type::SetKey), key(key) {}

	const float key;
};

class SessionEvent : public Event
{
public:
	SessionEvent(Type type, IMFMediaEvent* mediaEvent) : Event(type), m_mediaEvent(mediaEvent) {}

	virtual std::tstring toString() const override;

protected:
	CComPtr<IMFMediaEvent> m_mediaEvent;
};

class State : public tsm::State<Context, Event, State>
{
public:
	State(State* masterState = nullptr) : tsm::State<Context, Event, State>(masterState) {}
};

class StoppedState : public State
{
public:
	virtual HRESULT handleEvent(Context* context, Event* event, State** nextState) override;
};

class PlayingState : public State
{
public:
	// Stop playing on shutdown of StateMachine.
	virtual bool _isExitCalledOnShutdown() const override { return true; }

	virtual HRESULT entry(Context* context, Event*, State*) override;
	virtual HRESULT exit(Context* context, Event*, State*) override;
	virtual HRESULT handleEvent(Context* context, Event* event, State** nextState) override;
};

class PausedState : public State
{
public:
	PausedState(State* masterState) : State(masterState) {}

	virtual HRESULT entry(Context* context, Event*, State*) override;
	virtual HRESULT exit(Context* context, Event*, State*) override;
	virtual HRESULT handleEvent(Context* context, Event* event, State** nextState) override;
};
