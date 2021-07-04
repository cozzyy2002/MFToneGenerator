#include "pch.h"
#include "StateMachine.h"

template<typename T>
struct ValueName {
    T value;
    LPCTSTR name;

    template<size_t size>
    static const ValueName* find(const ValueName (&list)[size], T& value) {
        for(auto& d : list) {
            if(d.value == value) { return &d; }
        }
        return nullptr;
    }
};

Context::Context(HWND hWnd, UINT msg)
{
    HR_EXPECT_OK(MFStartup(MF_VERSION));

    m_stateMonitor.reset(new StateMonitor());
    m_stateMachine.reset(tsm::IStateMachine::create(hWnd, msg));
}

Context::~Context()
{
    HR_EXPECT_OK(MFShutdown());
}

HRESULT Context::setKey(Event* event)
{
    if(event->type == Event::Type::SetKey) {
        auto ev = (SetKeyEvent*)event;
        ev->key;
        return S_OK;
    } else {
        return E_UNEXPECTED;
    }
}

HRESULT Context::setupSession()
{
    // Media Session should not be created yet.
    HR_ASSERT(!m_session, E_ILLEGAL_METHOD_CALL);

    CComPtr<IMFSourceResolver> resolver;
    HR_ASSERT_OK(MFCreateSourceResolver(&resolver));

    MF_OBJECT_TYPE objectType;
    CComPtr<IUnknown> unk;
    static LPCWSTR url =
        //L"D:\\home\\MyMusic\\Rainbow\\Ritchie Blackmore's Rainbow\\01 Man on the Silver Mountain.wma"
        //L"D:\\home\\MyMusic\\iAUDIO - Friends & Lovers.mp3"
        L"D:\\home\\MyMusic\\Leon Russell\\Carney\\07 Carney.wma"
        ;
    HR_ASSERT_OK(resolver->CreateObjectFromURL(url, MF_RESOLUTION_MEDIASOURCE, nullptr, &objectType, &unk));
    HR_ASSERT_OK(unk->QueryInterface(&m_source));

    HR_ASSERT_OK(MFCreateMediaSession(nullptr, &m_session));
    m_callback = new MediaSessionCallback(this, m_session);
    HR_ASSERT_OK(m_callback->beginGetEvent());

    CComPtr <IMFTopology> topology;
    HR_ASSERT_OK(MFCreateTopology(&topology));

    // Create Topology Node for Media Source and Media Sink.
    CComPtr<IMFPresentationDescriptor> pd;
    HR_ASSERT_OK(m_source->CreatePresentationDescriptor(&pd));
    DWORD sdCount;
    HR_ASSERT_OK(pd->GetStreamDescriptorCount(&sdCount));
    for(DWORD isd = 0; isd < sdCount; isd++) {
        CComPtr<IMFStreamDescriptor> sd;
        BOOL isSelected;
        HR_ASSERT_OK(pd->GetStreamDescriptorByIndex(isd, &isSelected, &sd));

        if(isSelected) {
            // Create Topology Node for Media Source.
            CComPtr<IMFTopologyNode> sourceNode;
            HR_ASSERT_OK(MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &sourceNode));
            HR_ASSERT_OK(sourceNode->SetUnknown(MF_TOPONODE_SOURCE, m_source));
            HR_ASSERT_OK(sourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pd));
            HR_ASSERT_OK(sourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, sd));
            HR_ASSERT_OK(topology->AddNode(sourceNode));

            // Create Topology Node for Media Sink.
            CComPtr<IMFActivate> activate;
            HR_ASSERT_OK(MFCreateAudioRendererActivate(&activate));
            CComPtr<IMFTopologyNode> sinkNode;
            HR_ASSERT_OK(MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &sinkNode));
            HR_ASSERT_OK(sinkNode->SetObject(activate));
            HR_ASSERT_OK(topology->AddNode(sinkNode));

            sourceNode->ConnectOutput(0, sinkNode, 0);
            break;
        }
    }
    HR_ASSERT_OK(m_session->SetTopology(0, topology));

    return S_OK;
}

HRESULT Context::startSession()
{
    PROPVARIANT startTime;
    PropVariantInit(&startTime);
    HR_ASSERT_OK(m_session->Start(nullptr, &startTime));

    return S_OK;
}

HRESULT Context::stopSession()
{
    // Media Session should be created.
    HR_ASSERT(m_session, E_ILLEGAL_METHOD_CALL);

    return HR_EXPECT_OK(m_session->Stop());
}

HRESULT Context::closeSession()
{
    // Media Session should be created.
    HR_ASSERT(m_session, E_ILLEGAL_METHOD_CALL);

    return HR_EXPECT_OK(m_session->Close());
}

HRESULT Context::shutdownSession()
{
    if(m_source) {
        HR_EXPECT_OK(m_source->Shutdown());
        m_source.Release();
    }
    if(m_session) {
        HR_EXPECT_OK(m_session->Shutdown());
        m_session.Release();
    }
    m_callback.Release();

    return S_OK;
}

HRESULT Context::pauseSession()
{
    // Media Session should be created.
    HR_ASSERT(m_session, E_ILLEGAL_METHOD_CALL);

    return HR_EXPECT_OK(m_session->Pause());
}

HRESULT Context::resumeSession()
{
    return startSession();
}

HRESULT Context::MediaSessionCallback::beginGetEvent()
{
    return m_session->BeginGetEvent(this, nullptr);
}

typedef struct {
    Event::Type eventType;
    LPCTSTR name;
    DWORD mediaSessionEvent;    // Used by Media Session Event.
} EventData;

static const EventData eventDataList[] = {
    // UI Events
#define EVENT_DATA0(x) { Event::Type::x, _T(#x), MEUnknown }
    EVENT_DATA0(Unknown),
    EVENT_DATA0(StartStop),
    EVENT_DATA0(PauseResume),
    EVENT_DATA0(SetKey),
#undef EVENT_DATA0

    // Media Session Events
#define EVENT_DATA(x) { Event::Type::x, _T(#x), x }
    EVENT_DATA(MEEndOfPresentation),
    EVENT_DATA(MEError),
    EVENT_DATA(MESessionClosed),
    EVENT_DATA(MESessionEnded),
    EVENT_DATA(MESessionNotifyPresentationTime),
    EVENT_DATA(MESessionStarted),
    EVENT_DATA(MESessionPaused),
    EVENT_DATA(MESessionStopped),
    EVENT_DATA(MESessionTopologySet),
    EVENT_DATA(MESessionTopologyStatus),
    EVENT_DATA(MESessionCapabilitiesChanged),
#undef EVENT_DATA
};

HRESULT Context::MediaSessionCallback::Invoke(__RPC__in_opt IMFAsyncResult* pAsyncResult)
{
    CComPtr<IMFMediaEvent> mediaEvent;
    HR_ASSERT_OK(m_session->EndGetEvent(pAsyncResult, &mediaEvent));

    MediaEventType mediaEventType;
    HR_ASSERT_OK(mediaEvent->GetType(&mediaEventType));

    const EventData* data = nullptr;
    for(auto& d : eventDataList) {
        if(mediaEventType == d.mediaSessionEvent) {
            data = &d;
            break;
        }
    }
    if(data) {
        HR_ASSERT_OK(m_context->triggerEvent(new SessionEvent(data->eventType, mediaEvent)));
    } else {
        log(_T("Unknown Media Session Event: MediaEventType=%d"), mediaEventType);
    }

    // Prepare for next Session Event except for MESessionClosed(Last of Media Session Events.).
    if(!data || data->eventType != Event::Type::MESessionClosed) {
        HR_ASSERT_OK(beginGetEvent());
    }

    return S_OK;
}

std::tstring Event::toString() const
{
    if(m_string.empty()) {
        auto baseString = tsm::Event<Context>::toString();
        if((Type::Unknown <= type) && ((size_t)type < ARRAYSIZE(eventDataList))) {
            m_string = format(_T("%s:%s"), baseString.c_str(), eventDataList[(int)type].name);
        } else {
            m_string = format(_T("%s:Unknown(%d)"), baseString.c_str(), type);
        }
    }
    return m_string;
}

std::tstring SessionEvent::toString() const
{
    static const ValueName<UINT32> topoStatus[] = {
#define VALUE_NAME(x) { x, _T(#x) }
        VALUE_NAME(MF_TOPOSTATUS_INVALID),
        VALUE_NAME(MF_TOPOSTATUS_READY),
        VALUE_NAME(MF_TOPOSTATUS_STARTED_SOURCE),
        VALUE_NAME(MF_TOPOSTATUS_DYNAMIC_CHANGED),
        VALUE_NAME(MF_TOPOSTATUS_SINK_SWITCHED),
        VALUE_NAME(MF_TOPOSTATUS_ENDED),
#undef VALUE_NAME
    };

    if(m_string.empty()) {
        switch(type) {
        case Type::MESessionNotifyPresentationTime:
            {
                UINT64 time;
                HR_EXPECT_OK(m_mediaEvent->GetUINT64(MF_EVENT_START_PRESENTATION_TIME, &time));
                m_string = format(_T("%s(Time = %f sec)"), Event::toString().c_str(), (float)time / 10000);
            }
            break;
        case Type::MESessionCapabilitiesChanged:
            {
                UINT32 caps, delta;
                HR_EXPECT_OK(m_mediaEvent->GetUINT32(MF_EVENT_SESSIONCAPS, &caps));
                HR_EXPECT_OK(m_mediaEvent->GetUINT32(MF_EVENT_SESSIONCAPS_DELTA, &delta));
                m_string = format(_T("%s(Caps/Delta/And = 0x%08x/0x%08x/0x%08x)"), Event::toString().c_str(), caps, delta, caps & delta);
            }
            break;
        case Type::MESessionTopologyStatus:
            {
                UINT32 status;
                auto hr = HR_EXPECT_OK(m_mediaEvent->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &status));
                if(SUCCEEDED(hr)) {
                    auto x = ValueName<UINT32>::find(topoStatus, status);
                    std::tstring strStatus(x ? x->name : (format(_T("Unknown(%d)"), status).c_str()));
                    m_string = format(_T("%s:%s"), Event::toString().c_str(), strStatus.c_str());
                } else {
                    Event::toString();
                }
            }
            break;
        default:
            Event::toString();
            break;
        }
    }
    return m_string;
}

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

void Context::StateMonitor::onEventTriggered(Context* context, Event* event)
{
    log(_T("onEventTriggered(%s)"), event->toString().c_str());
}

void Context::StateMonitor::onStateChanged(Context* context, Event* event, State* previous, State* next)
{
#define TO_STRING(x) x ? x->toString().c_str() : _T("null")
    log(_T("onStateChanged(%s, %s, %s)"), TO_STRING(event), TO_STRING(previous), TO_STRING(next));
#undef TO_STRING
}
