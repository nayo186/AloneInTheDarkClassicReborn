///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Build configuration and compile-time options
///////////////////////////////////////////////////////////////////////////////

#pragma once

#if __cplusplus
#include <vector>
#include <string>
#if __has_include(<format>)
#include <format>
#endif
#include <optional>
#endif

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

// FITD_DEBUGGER is enabled only in Debug builds
// Release builds will have no debug menu, no collision visualization, no ImGui overhead
#ifdef _DEBUG
    #define FITD_DEBUGGER
#endif

#include "types.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#ifdef WIN32
#include <search.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include <assert.h>

#ifdef _DEBUG
#define ASSERT(exp) assert(exp)
#else
#define ASSERT(exp)
#endif

#ifdef _DEBUG
#define ASSERT_PTR(exp) assert(exp)
#else
#define ASSERT_PTR(exp)
#endif

