#include "pch.h"

#include "Context.h"
#include "Event.h"
#include "State.h"

HRESULT StoppedState::handleEvent(Context* context, Event* event, State** nextState)
{
    switch(event->type) {
    case Event::Type::StartStop:
        *nextState = new PlayingState();
        break;
    case Event::Type::SetKey:
        HR_ASSERT_OK(context->setKey(event));
        *nextState = new PlayingState();
        break;
    case Event::Type::MESessionStopped:
    case Event::Type::MESessionEnded:
        HR_ASSERT_OK(context->closeSession());
        break;
    case Event::Type::MESessionClosed:
        HR_ASSERT_OK(context->shutdownSession());
        break;
    }
    return S_OK;
}

HRESULT PlayingState::entry(Context* context, Event*, State*)
{
    return HR_EXPECT_OK(context->setupSession());
}

HRESULT PlayingState::exit(Context* context, Event*, State*)
{
    return S_OK;
}

HRESULT PlayingState::handleEvent(Context* context, Event* event, State** nextState)
{
    switch(event->type) {
    case Event::Type::MESessionTopologySet:
        HR_ASSERT_OK(context->startSession());
        break;
    case Event::Type::StartStop:
        HR_ASSERT_OK(context->stopSession());
        *nextState = new StoppedState();
        break;
    case Event::Type::MESessionEnded:
        HR_ASSERT_OK(context->closeSession());
        *nextState = new StoppedState();
        break;
    case Event::Type::PauseResume:
        *nextState = new PausedState(this);
        break;
    case Event::Type::SetKey:
        HR_ASSERT_OK(context->setKey(event));
        break;
    case Event::Type::MESessionStarted:
        break;
    }
    return S_OK;
}

HRESULT PausedState::entry(Context* context, Event*, State*)
{
    return HR_EXPECT_OK(context->pauseSession());
}

HRESULT PausedState::exit(Context* context, Event*, State*)
{
    return S_OK;
}

HRESULT PausedState::handleEvent(Context* context, Event* event, State** nextState)
{
    switch(event->type) {
    case Event::Type::PauseResume:
        HR_ASSERT_OK(context->resumeSession());
        *nextState = getMasterState();
        break;
    default:
        // Other Events are handled by PlayingState.
        return S_FALSE;
    }
    return S_OK;
}
