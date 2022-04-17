#include "pch.h"

#include "Context.h"
#include "Event.h"
#include "State.h"

namespace statemachine {

HRESULT StoppedState::handleEvent(Context* context, Event* event, State** nextState)
{
    switch(event->type) {
    case Event::Type::StartTone:
        {
            auto ev = (StartToneEvent*)event;
            context->setPcmData(ev->pcmData);
        }
        HR_EXPECT_OK(context->setupSession());
        break;
    case Event::Type::StartFile:
    {
        auto ev = (StartFileEvent*)event;
        context->setMediaFileName(ev->fileName.c_str(), ev->hwnd);
    }
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
    DWORD caps;
    HR_ASSERT_OK(context->getSession()->GetSessionCapabilities(&caps));
    bool canPause = caps & MFSESSIONCAP_PAUSE;
    context->callback([canPause](Context::ICallback* callback) { callback->onStarted(canPause); });
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
    case Event::Type::Stop:
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
    case Event::Type::MEError:
        {
            HR_ASSERT_OK(context->stopSession());
            callErrorCallback(context, (SessionEvent*)event);
            break;
        }
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

void State::callErrorCallback(Context* context, SessionEvent* sessionEvent) const
{
    HRESULT hr;
    HR_EXPECT_OK(sessionEvent->getMediaEvent()->GetStatus(&hr));
    context->callback([hr](Context::ICallback* callback) { callback->onError(_T("MediaSession"), hr, _T("MEError")); });
}

}
