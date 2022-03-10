#include "pch.h"

#include "Event.h"

namespace statemachine {

static const Event::EventData eventDataList[] = {
    // UI Events
#define EVENT_DATA0(x) { Event::Type::x, MEUnknown, _T(#x) }
    EVENT_DATA0(Unknown),
    EVENT_DATA0(StartTone),
    EVENT_DATA0(StartFile),
    EVENT_DATA0(Stop),
    EVENT_DATA0(PauseResume),
#undef EVENT_DATA0

    // Media Session Events
#define EVENT_DATA(x) { Event::Type::x, x, _T(#x) }
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

/*static*/ const Event::EventData* Event::find(MediaEventType value)
{
    const EventData* data = nullptr;
    for(auto& d : eventDataList) {
        if(d.mediaEventType == value) {
            data = &d;
            break;
        }
    }
    return data;
}

/*static*/ const Event::EventData* Event::find(Event::Type value)
{
    const EventData* data = nullptr;
    if((size_t)value < ARRAYSIZE(eventDataList)) {
        data = &eventDataList[(size_t)value];
    }
    return data;
}

std::tstring Event::toString() const
{
    if(m_string.empty()) {
        auto baseString = BaseClass::toString();
        auto data = find(type);
        if(data) {
            m_string = format(_T("%s:%s"), baseString.c_str(), data->name);
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
                    m_string = format(_T("%s: IMFMediaEvent::GetUINT32(MF_EVENT_TOPOLOGY_STATUS) failed. Error=0x%p"), Event::toString().c_str(), hr);
                }
            }
            break;
        case Type::MEError:
            {
                HRESULT hrStatus;
                auto hr = HR_EXPECT_OK(m_mediaEvent->GetStatus(&hrStatus));
                if(SUCCEEDED(hr)) {
                    m_string = format(_T("%s: Error=0x%p"), Event::toString().c_str(), hrStatus);
                } else {
                    m_string = format(_T("%s: IMFMediaEvent::GetStatus() failed. Error=0x%p"), Event::toString().c_str(), hr);
                }
            }
        default:
            Event::toString();
            break;
        }
    }
    return m_string;
}

}
