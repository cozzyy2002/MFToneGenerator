#pragma once

#include "Utils.h"

class Context;
class State;

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
