#pragma once

#include "PcmData/PcmData.h"
#include "Utils.h"

namespace statemachine {

class Context;
class State;

class Event : public tsm::Event<Context>, public StringFormatter
{
public:
	enum class Type
	{
		Unknown,

		// Control Events from UI.
		StartTone,
		StartFile,
		Stop,
		PauseResume,

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

	struct EventData {
		Type eventType;
		MediaEventType mediaEventType;
		LPCTSTR name;
	};

	static const EventData* find(Type type);
	static const EventData* find(MediaEventType mediaEventType);

	Event(Type type) : type(type) {}
	const Type type;

	virtual std::tstring toString() const override;

protected:
	using BaseClass = tsm::Event<Context>;
	mutable std::tstring m_string;
};

class StartToneEvent : public Event
{
public:
	StartToneEvent(std::shared_ptr<IPcmData>& pcmData, HWND hwnd = NULL) : Event(Type::StartTone), pcmData(pcmData), hwnd(hwnd) {}

	std::shared_ptr<IPcmData> pcmData;
	HWND hwnd;
};

class StartFileEvent : public Event
{
public:
	StartFileEvent(LPCTSTR fileName, HWND hwnd) : Event(Type::StartFile), fileName(fileName), hwnd(hwnd) {}

	const std::tstring fileName;
	HWND hwnd;
};

class SessionEvent : public Event
{
public:
	SessionEvent(Type type, IMFMediaEvent* mediaEvent) : Event(type), m_mediaEvent(mediaEvent) {}

	IMFMediaEvent* getMediaEvent() const { return m_mediaEvent; }
	virtual std::tstring toString() const override;

protected:
	CComPtr<IMFMediaEvent> m_mediaEvent;
};

class TopologyStatusEvent : public SessionEvent
{
public:
	TopologyStatusEvent(IMFMediaEvent* mediaEvent, UINT32 status) : SessionEvent(Type::MESessionTopologyStatus, mediaEvent), status(status) {}

	const UINT32 status;
};
}
