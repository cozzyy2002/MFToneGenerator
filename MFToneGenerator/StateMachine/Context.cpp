#include "pch.h"

#include "Context.h"
#include "Event.h"
#include "State.h"

static void print(IMFMediaSource*);
static void print(IMFStreamDescriptor*, DWORD index);
static void print(IMFAttributes* , LPCTSTR title);

Context::Context(HWND hWnd, UINT msg)
    : m_callback(nullptr)
{
    HR_EXPECT_OK(MFStartup(MF_VERSION));

    m_stateMonitor.reset(new StateMonitor());
    m_stateMachine.reset(tsm::IStateMachine::create(hWnd, msg));
}

Context::~Context()
{
    HR_EXPECT_OK(MFShutdown());
}

void Context::callback(std::function<void(ICallback*)> func)
{
    if(m_callback) {
        func(m_callback);
    }
}

HRESULT Context::setup()
{
    return HR_EXPECT_OK(BaseClass::setup(new StoppedState()));
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
    auto audioFileName = m_audioFileName.c_str();
    CT2W url(audioFileName);
    auto hr = HR_EXPECT_OK(resolver->CreateObjectFromURL(url, MF_RESOLUTION_MEDIASOURCE, nullptr, &objectType, &unk));
    if(FAILED(hr)) {
        auto msg(format(hr, audioFileName));
        log(_T("%s: %s"), audioFileName, msg.c_str());
        callback([hr, &msg](ICallback* callback) { callback->onError(_T("CreateObjectFromURL()"), hr, msg.c_str()); });
        return hr;
    }
    HR_ASSERT_OK(unk->QueryInterface(&m_source));
    print(m_source);

    HR_ASSERT_OK(MFCreateMediaSession(nullptr, &m_session));
    m_mediaSessionCallback = new MediaSessionCallback(this, m_session);
    HR_ASSERT_OK(m_mediaSessionCallback->beginGetEvent());

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
    m_mediaSessionCallback.Release();

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


void print(IMFMediaSource* mediaSource)
{
    Logger log;

    struct Charact {
        MFMEDIASOURCE_CHARACTERISTICS charact;
        LPCTSTR name;
    };
    static const Charact charactList[] = {
#define ITEM(x) { MFMEDIASOURCE_##x, _T(#x) }
        ITEM(IS_LIVE),
        ITEM(CAN_SEEK),
        ITEM(CAN_PAUSE),
        ITEM(HAS_SLOW_SEEK),
        ITEM(HAS_MULTIPLE_PRESENTATIONS),
        ITEM(CAN_SKIPFORWARD),
        ITEM(CAN_SKIPBACKWARD),
        ITEM(DOES_NOT_USE_NETWORK),
#undef ITEM
    };
    DWORD characts;
    mediaSource->GetCharacteristics(&characts);
    std::tstring strCharacts;
    std::tstring separator;
    for(auto& charact : charactList) {
        if(charact.charact == (charact.charact & characts)) {
            strCharacts += separator + charact.name;
            if(separator.empty()) { separator = _T(", "); }
            continue;
        }
    }

    CComPtr<IMFPresentationDescriptor> pd;
    mediaSource->CreatePresentationDescriptor(&pd);
    DWORD sdCount;
    pd->GetStreamDescriptorCount(&sdCount);
    log.log(_T("IMFMediaSource: %d Stream Desctiptors, Characteristics=0x%08x [%s]"), sdCount, characts, strCharacts.c_str());
    print(pd, _T("Attributes of IMFPresentationDescriptor"));
    for(DWORD i = 0; i < sdCount; i++) {
        BOOL isSelected;
        CComPtr<IMFStreamDescriptor> sd;
        pd->GetStreamDescriptorByIndex(i, &isSelected, &sd);
        print(sd, i);
    }
}

void print(IMFStreamDescriptor* sd, DWORD index)
{
    Logger log;
    DWORD id;
    sd->GetStreamIdentifier(&id);
    log.log(_T("IMFStreamDescriptor %d: Identifier=%d"), index, id);
    print(sd, _T("Attributes of IMFStreamDescriptor"));
}

void print(IMFAttributes* attr, LPCTSTR title)
{
    Logger log;
    UINT32 count;
    attr->GetCount(&count);

    log.log(_T("%s: %d Items"), title, count);
    for(UINT32 i = 0; i < count; i++) {
        GUID key;
        PROPVARIANT value;
        attr->GetItemByIndex(i, &key, &value);

        OLECHAR wstrKey[50];
        StringFromGUID2(key, wstrKey, ARRAYSIZE(wstrKey));
        CW2T tstrKey(wstrKey);

        std::tstring strValue;
        switch(value.vt) {
        case VT_UI4:
            strValue = log.format(_T("VT_UI4(%d) %u"), VT_UI4, value.uintVal);
            break;
        case VT_UI8:
            strValue = log.format(_T("VT_UI8(%d) %u"), VT_UI8, value.bVal);
            break;
        case VT_LPWSTR:
            {
                CW2T tValue(value.pwszVal);
                strValue = log.format(_T("VT_LPWSTR(%d) `%s`"), VT_LPWSTR, (LPCTSTR)tValue);
            }
            break;
        default:
            strValue = log.format(_T("Unknown type %d"), value.vt);
            break;
        }

        log.log(_T("  %2d %s %s"), i, (LPCTSTR)tstrKey, strValue.c_str());
        PropVariantClear(&value);
    }
}
