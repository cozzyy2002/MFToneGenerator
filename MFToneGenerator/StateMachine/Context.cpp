#include "pch.h"

#include "Context.h"
#include "Event.h"
#include "State.h"

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
    static LPCWSTR url = L"D:\\home\\MyMusic\\"
        //L"Rainbow\\Ritchie Blackmore's Rainbow\\01 Man on the Silver Mountain.wma"
        //L"iAUDIO - Friends & Lovers.mp3"
        L"Leon Russell\\Carney\\07 Carney.wma"
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

HRESULT Context::MediaSessionCallback::Invoke(__RPC__in_opt IMFAsyncResult* pAsyncResult)
{
    CComPtr<IMFMediaEvent> mediaEvent;
    HR_ASSERT_OK(m_session->EndGetEvent(pAsyncResult, &mediaEvent));

    MediaEventType mediaEventType;
    HR_ASSERT_OK(mediaEvent->GetType(&mediaEventType));

    auto data = Event::find(mediaEventType);
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
