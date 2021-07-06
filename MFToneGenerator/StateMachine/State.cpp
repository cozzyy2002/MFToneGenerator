#include "pch.h"

#include "Context.h"
#include "Event.h"
#include "State.h"

HRESULT StoppedState::handleEvent(Context* context, Event* event, State** nextState)
{
    switch(event->type) {
    case Event::Type::StartStop:
        HR_EXPECT_OK(context->setupSession());
        break;
    case Event::Type::SetKey:
        HR_ASSERT_OK(context->setKey(event));
        HR_EXPECT_OK(context->setupSession());
        break;
    case Event::Type::MESessionTopologySet:
        HR_ASSERT_OK(context->startSession());
        break;
    case Event::Type::MESessionStarted:
        *nextState = new PlayingState();
        break;
    }
    return S_OK;
}

HRESULT PlayingState::entry(Context* context, Event* event, State* previousState)
{
    context->callback([](Context::ICallback* callback) { callback->onStarted(); });
    return S_OK;
}

HRESULT PlayingState::exit(Context* context, Event* event, State* nextState)
{
    context->callback([](Context::ICallback* callback) { callback->onStopped(); });
    return S_OK;
}

HRESULT PlayingState::handleEvent(Context* context, Event* event, State** nextState)
{
    switch(event->type) {
    case Event::Type::StartStop:
        HR_ASSERT_OK(context->stopSession());
        break;
    case Event::Type::MESessionStopped:
    case Event::Type::MESessionEnded:
        HR_ASSERT_OK(context->closeSession());
        break;
    case Event::Type::MESessionClosed:
        HR_ASSERT_OK(context->shutdownSession());
        *nextState = new StoppedState();
        break;
    case Event::Type::PauseResume:
        HR_ASSERT_OK(context->pauseSession());
        break;
    case Event::Type::MESessionPaused:
        *nextState = new PausedState(this);
        break;
    case Event::Type::SetKey:
        HR_ASSERT_OK(context->setKey(event));
        break;
    }
    return S_OK;
}

HRESULT PausedState::entry(Context* context, Event*, State*)
{
    context->callback([](Context::ICallback* callback) { callback->onPaused(); });
    return S_OK;
}

HRESULT PausedState::exit(Context* context, Event* event, State* nextState)
{
    // Note: Next state might be StoppedState if Stopped while Pausing.
    //       In this case, Do not call onResumed().
    if(nextState == getMasterState()) {
        context->callback([](Context::ICallback* callback) { callback->onResumed(); });
    }
    return S_OK;
}

HRESULT PausedState::handleEvent(Context* context, Event* event, State** nextState)
{
    switch(event->type) {
    case Event::Type::PauseResume:
        HR_ASSERT_OK(context->resumeSession());
        break;
    case Event::Type::MESessionStarted:
        *nextState = getMasterState();
        break;
    default:
        // Other Events are handled by PlayingState.
        return S_FALSE;
    }
    return S_OK;
}
