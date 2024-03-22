// dawn/include/dawn/debug.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cassert>
#include <syslog.h>

#include "dawn/porting/config.hxx"

// Info messages

#ifdef CONFIG_DAWN_DEBUG_INFO
#  define DAWNINFO(format, ...)                                             \
    syslog(LOG_MAKEPRI(LOG_USER, LOG_INFO), "dawn: " format, ##__VA_ARGS__)
#else
#  define DAWNINFO(format, ...)
#endif

// Warning messages

#ifdef CONFIG_DAWN_DEBUG_WARN
#  define DAWNWARN(format, ...)                                                \
    syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING), "dawn: " format, ##__VA_ARGS__)
#else
#  define DAWNWARN(format, ...)
#endif

// Error messages

#ifdef CONFIG_DAWN_DEBUG_ERROR
#  define DAWNERR(format, ...)                                             \
    syslog(LOG_MAKEPRI(LOG_USER, LOG_ERR), "dawn: " format, ##__VA_ARGS__)
#else
#  define DAWNERR(format, ...)
#endif

// Custom Dawn assertions.
//
// Input parameters:
//   a - assertion condition to check
//   msg - additional message about what is wrong

#define DAWNASSERT(a, msg) assert(a)
