// dawn/src/proto/can/can_internal.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <stdint.h>
#include <time.h>

#include "dawn/porting/can.hxx"
#include "dawn/proto/can/can.hxx"

#ifdef CONFIG_DAWN_PROTO_CAN_EXTID
#  define CAN_EXTID_FLAG 1
#else
#  define CAN_EXTID_FLAG 0
#endif

#define CAN_SEG_SEQ_MAX 0x80

#ifdef DAWN_PROTO_HAS_ISOTP
static inline uint64_t canTimestampUs()
{
  struct timespec ts;

  clock_systime_timespec(&ts);
  return 1000000ull * ts.tv_sec + ts.tv_nsec / 1000;
}
#endif
