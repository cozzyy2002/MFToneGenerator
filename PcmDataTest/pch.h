#pragma once

#include <windows.h>
#include <Shlwapi.h>
#include <mmreg.h>
#include <tchar.h>
#include <atlbase.h>

#include <memory>
#include <string>

namespace std {
#if defined(UNICODE)
using tstring = std::wstring;
#else
using tstring = std::string;
#endif
}

#define tsm_STATE_MACHINE_EXPORT
#include "../../tsm/public/include/StateMachine/Assert.h"

#include "../../tsm/public/include/StateMachine/Unknown.h"

