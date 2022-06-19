// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"

#include <mfidl.h>
#include <mfapi.h>
#include <Mferror.h>
#include <stdio.h>

#include <StateMachine/stdafx.h>
#include <StateMachine/Context.h>
#include <StateMachine/Event.h>
#include <StateMachine/State.h>
#include <StateMachine/Assert.h>
#include <StateMachine/StateMonitor.h>

#include <d2d1.h>
#include <wincodec.h>

#endif //PCH_H
