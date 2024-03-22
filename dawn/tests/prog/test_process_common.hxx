// dawn/tests/prog/test_process_common.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <unistd.h>

#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "test_common.hxx"

using namespace dawn;

namespace dawn
{
static constexpr auto PROCESS_DUMMYIO1 = CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto PROCESS_VIRTIO1 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto PROCESS_DUMMYIO2 = CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 2);
static constexpr auto PROCESS_VIRTIO2 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 2);

static constexpr useconds_t PROCESS_NOTIFY_WAIT_US = 6000;

static inline void process_set_dummy_data(CIODummyNotify &src, int32_t base)
{
  io_sdata_t<int32_t, 10, 1> data;
  size_t i;

  for (i = 0; i < 10; i++)
    {
      data(i, 0) = base + i;
    }

  TEST_ASSERT_EQUAL(OK, src.start());
  TEST_ASSERT_EQUAL(OK, src.setData(data));
  usleep(PROCESS_NOTIFY_WAIT_US);
  TEST_ASSERT_EQUAL(OK, src.stop());
}
} // namespace dawn

#define DEFINE_PROCESS_BIN(name, ProgClass) \
  uint32_t name[] = {                       \
    ProgClass::objectId(0),                 \
    1,                                      \
    ProgClass::cfgIdIOBind(),               \
    PROCESS_DUMMYIO1,                       \
    PROCESS_VIRTIO1,                        \
  }

// Shared Test Data

extern uint32_t g_cfg_process_dummy1[];
extern uint32_t g_cfg_process_virt1[];
extern uint32_t g_cfg_process_dummy2[];
extern uint32_t g_cfg_process_virt2[];
