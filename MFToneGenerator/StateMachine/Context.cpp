#include "pch.h"

#include "Context.h"
#include "Event.h"
#include "State.h"
#include "ToneMediaSource/ToneMediaSource.h"

namespace statemachine {

static void print(IMFMediaSource*);
static void print(IMFStreamDescriptor*, DWORD index);
static void print(IMFAttributes*, LPCTSTR title);
static std::tstring guidToString(REFGUID guid);

/*static*/ IContext* IContext::create(HWND hWnd, UINT msg)
{
    return new Context(hWnd, msg);
}

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

HRESULT Context::shutdown()
{
    return HR_EXPECT_OK(BaseClass::shutdown());
}

HRESULT Context::startTone(std::shared_ptr<IPcmData>& pcmData, HWND hwnd /*= NULL*/)
{
    return HR_EXPECT_OK(triggerEvent(new StartToneEvent(pcmData, hwnd)));
}

HRESULT Context::startFile(LPCTSTR fileName, HWND hwnd)
{
    return HR_EXPECT_OK(triggerEvent(new StartFileEvent(fileName, hwnd)));
}

HRESULT Context::stop()
{
    return HR_EXPECT_OK(triggerEvent(new Event(Event::Type::Stop)));
}

HRESULT Context::pauseResume()
{
    return HR_EXPECT_OK(triggerEvent(new Event(Event::Type::PauseResume)));
}

HRESULT Context::setupPcmDataSession(std::shared_ptr<IPcmData>& pcmData, HWND hwnd /*= NULL*/)
{
    CComPtr<IMFMediaSource> source(new ToneMediaSource(pcmData));
    return setupSession(source, hwnd);
}

HRESULT Context::setupMediaFileSession(LPCTSTR fileName, HWND hwnd)
{
    CComPtr<IMFSourceResolver> resolver;
    HR_ASSERT_OK(MFCreateSourceResolver(&resolver));

    MF_OBJECT_TYPE objectType;
    CComPtr<IUnknown> unk;
    CT2W url(fileName);
    auto hr = HR_EXPECT_OK(resolver->CreateObjectFromURL(url, MF_RESOLUTION_MEDIASOURCE, nullptr, &objectType, &unk));
    if(FAILED(hr)) {
        auto msg(format(hr, fileName));
        log(_T("%s: %s"), fileName, msg.c_str());
        callback([hr, &msg](ICallback* callback) { callback->onError(_T("CreateObjectFromURL()"), hr, msg.c_str()); });
        return hr;
    }

    CComPtr<IMFMediaSource> mediaSource;
    HR_ASSERT_OK(unk->QueryInterface(&mediaSource));
    return setupSession(mediaSource, hwnd);
}

HRESULT Context::setupSession(IMFMediaSource* mediaSource, HWND hwnd /*= NULL*/)
{
    // Media Session should not be created yet.
    HR_ASSERT(!m_session, E_ILLEGAL_METHOD_CALL);

    HR_ASSERT_OK(MFCreateMediaSession(nullptr, &m_session));
    m_mediaSessionCallback = new MediaSessionCallback(this, m_session);
    HR_ASSERT_OK(m_mediaSessionCallback->beginGetEvent());

    CComPtr <IMFTopology> topology;
    HR_ASSERT_OK(MFCreateTopology(&topology));

    m_source = mediaSource;
    print(m_source);

    // Create Topology Node for Media Source and Media Sink.
    CComPtr<IMFPresentationDescriptor> pd;
    HR_ASSERT_OK(m_source->CreatePresentationDescriptor(&pd));
    DWORD sdCount;
    HR_ASSERT_OK(pd->GetStreamDescriptorCount(&sdCount));
    for(DWORD isd = 0; isd < sdCount; isd++) {
        CComPtr<IMFStreamDescriptor> sd;
        BOOL isSelected;
        HR_ASSERT_OK(pd->GetStreamDescriptorByIndex(isd, &isSelected, &sd));
        if(!isSelected) { continue; }

        CComPtr<IMFMediaTypeHandler> mth;
        HR_ASSERT_OK(sd->GetMediaTypeHandler(&mth));
        GUID majorType;
        HR_ASSERT_OK(mth->GetMajorType(&majorType));
        CComPtr<IMFActivate> activate;
        if(majorType == MFMediaType_Audio) {
            // Create the audio renderer.
            HR_ASSERT_OK(MFCreateAudioRendererActivate(&activate));
        } else if((majorType == MFMediaType_Video) && hwnd) {
            // Create the video renderer, if the rendering window has been specified.
            HR_ASSERT_OK(MFCreateVideoRendererActivate(hwnd, &activate));
        }

        if(activate) {
            // Create Topology Node for Media Source.
            CComPtr<IMFTopologyNode> sourceNode;
            HR_ASSERT_OK(MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &sourceNode));
            HR_ASSERT_OK(sourceNode->SetUnknown(MF_TOPONODE_SOURCE, m_source));
            HR_ASSERT_OK(sourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pd));
            HR_ASSERT_OK(sourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, sd));
            HR_ASSERT_OK(topology->AddNode(sourceNode));

            // Create Topology Node for Media Sink.
            CComPtr<IMFTopologyNode> sinkNode;
            HR_ASSERT_OK(MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &sinkNode));
            HR_ASSERT_OK(sinkNode->SetObject(activate));
            HR_ASSERT_OK(topology->AddNode(sinkNode));

            sourceNode->ConnectOutput(0, sinkNode, 0);
        } else {
            // No appropriate renderer for this media stream.
            log(_T("Deselecting unsupported stream %d. Major Type = %s"), isd, guidToString(majorType).c_str());
            pd->DeselectStream(isd);
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
    print(pd, _T("IMFPresentationDescriptor"));
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
    CComPtr<IMFMediaTypeHandler> mth;
    sd->GetMediaTypeHandler(&mth);
    GUID majorType;
    mth->GetMajorType(&majorType);
    log.log(_T("IMFStreamDescriptor %d: Identifier=%d, MajorType=%s"), index, id, guidToString(majorType).c_str());
    print(sd, _T("IMFStreamDescriptor"));
    DWORD cmt;
    mth->GetMediaTypeCount(&cmt);
    for(DWORD i = 0; i < cmt; i++) {
        CComPtr<IMFMediaType> mt;
        mth->GetMediaTypeByIndex(i, &mt);
        auto title = log.format(_T("IMFMediaType[%d]"), i);
        print(mt, title.c_str());
    }
}

void print(IMFAttributes* attr, LPCTSTR title)
{
    Logger log;
    UINT32 count;
    attr->GetCount(&count);

    log.log(_T("Attributes of %s: %d Items"), title, count);
    for(UINT32 i = 0; i < count; i++) {
        GUID key;
        PROPVARIANT value;
        attr->GetItemByIndex(i, &key, &value);

        std::tstring strValue;
        switch(value.vt) {
        case MF_ATTRIBUTE_UINT32:
            strValue = log.format(_T("MF_ATTRIBUTE_UINT32(%d) = %u"), value.vt, value.uintVal);
            break;
        case MF_ATTRIBUTE_UINT64:
            strValue = log.format(_T("MF_ATTRIBUTE_UINT64(%d) = %d - %d"), value.vt, value.uhVal.HighPart, value.uhVal.LowPart);
            break;
        case MF_ATTRIBUTE_DOUBLE:
            strValue = log.format(_T("MF_ATTRIBUTE_DOUBLE(%d) = %f"), value.vt, value.dblVal);
            break;
        case MF_ATTRIBUTE_GUID:
            strValue = log.format(_T("MF_ATTRIBUTE_GUID(%d) = %s"), value.vt, guidToString(*value.puuid).c_str());
            break;
        case MF_ATTRIBUTE_STRING:
            {
                CW2T tValue(value.pwszVal);
                strValue = log.format(_T("MF_ATTRIBUTE_STRING(%d) = `%s`"), value.vt, (LPCTSTR)tValue);
            }
            break;
        case MF_ATTRIBUTE_BLOB:
            strValue = log.format(_T("MF_ATTRIBUTE_BLOB(%d)"), value.vt);
            break;
        case MF_ATTRIBUTE_IUNKNOWN:
            strValue = log.format(_T("MF_ATTRIBUTE_IUNKNOWN(%d) = 0x%p"), value.vt, value.punkVal);
            break;
        default:
            strValue = log.format(_T("Unknown type %d"), value.vt);
            break;
        }

        log.log(_T("  %2d %s:%s"), i, guidToString(key).c_str(), strValue.c_str());
        PropVariantClear(&value);
    }
}

struct GuidName {
    REFGUID guid;
    LPCTSTR name;
};

static const GuidName guidNames[] = {
#define ITEM(x) { x, _T(#x) }
    // General Format Attributes of Media Type.
    ITEM(MF_MT_ALL_SAMPLES_INDEPENDENT),
    ITEM(MF_MT_AM_FORMAT_TYPE),
    ITEM(MF_MT_COMPRESSED),
    ITEM(MF_MT_FIXED_SIZE_SAMPLES),
    ITEM(MF_MT_MAJOR_TYPE),
    ITEM(MF_MT_SAMPLE_SIZE),
    ITEM(MF_MT_SUBTYPE),

    // Audio Format Attributes of Media Type.
    ITEM(MF_MT_AUDIO_AVG_BYTES_PER_SECOND),
    ITEM(MF_MT_AUDIO_BITS_PER_SAMPLE),
    ITEM(MF_MT_AUDIO_BLOCK_ALIGNMENT),
    ITEM(MF_MT_AUDIO_CHANNEL_MASK),
    ITEM(MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND),
    ITEM(MF_MT_AUDIO_NUM_CHANNELS),
    ITEM(MF_MT_AUDIO_PREFER_WAVEFORMATEX),
    ITEM(MF_MT_AUDIO_SAMPLES_PER_SECOND),
    ITEM(MF_MT_AUDIO_VALID_BITS_PER_SAMPLE),
    ITEM(MF_MT_ORIGINAL_WAVE_FORMAT_TAG),

    // Video Format Attributes of Media Type.
    ITEM(MF_MT_AVG_BITRATE),
    ITEM(MF_MT_DEFAULT_STRIDE),
    ITEM(MF_MT_FRAME_RATE),
    ITEM(MF_MT_FRAME_SIZE),
    ITEM(MF_MT_INTERLACE_MODE),
    ITEM(MF_MT_VIDEO_ROTATION),
    ITEM(MF_MT_PIXEL_ASPECT_RATIO),

    // Major Media Types.
    ITEM(MFMediaType_Audio),
    ITEM(MFMediaType_Video),

    // Subtypes.
    ITEM(MFAudioFormat_PCM),
    ITEM(MFAudioFormat_MPEG),
    ITEM(MFVideoFormat_RGB24),
#undef ITEM
};

/*static*/ std::tstring guidToString(REFGUID guid)
{
    StringFormatter fmt;
    for(auto& x : guidNames) {
        if(x.guid == guid) {
            return fmt.format(_T("%s(%s)"), x.name, fmt.toString(guid).c_str());
        }
    }

    return fmt.toString(guid);
}

}
