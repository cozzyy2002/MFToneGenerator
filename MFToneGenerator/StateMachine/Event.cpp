#include "pch.h"

#include "Event.h"

/*static*/ const Event::EventData Event::eventDataList[] = {
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
