#pragma once

#include "Utils.h"

namespace statemachine {

class Context;
class Event;

class State : public tsm::State<Context, Event, State>
{
public:
	State(State* masterState = nullptr) : BaseClass(masterState) {}

protected:
	using BaseClass = tsm::State<Context, Event, State>;

	void callErrorCallback(Context* context, SessionEvent* sessionEvent) const;
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

	virtual HRESULT entry(Context* context, Event* event, State* previousState) override;
	virtual HRESULT exit(Context* context, Event* event, State* nextState) override;
	virtual HRESULT handleEvent(Context* context, Event* event, State** nextState) override;
};

class PausedState : public State
{
public:
	PausedState(State* masterState) : State(masterState) {}

	virtual HRESULT entry(Context* context, Event* event, State* previousState) override;
	virtual HRESULT exit(Context* context, Event* event, State* nextState) override;
	virtual HRESULT handleEvent(Context* context, Event* event, State** nextState) override;
};

}
