// dawn/tests/prog/test_process_common.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "test_process_common.hxx"

using namespace dawn;

uint32_t g_cfg_process_dummy1[] = {
  PROCESS_DUMMYIO1,
  3,
  CIODummyNotify::cfgIdDim(),
  10,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 10),
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  CIODummyNotify::cfgInterval(false),
  5000,
};

uint32_t g_cfg_process_virt1[] = {
  PROCESS_VIRTIO1,
  0,
};

uint32_t g_cfg_process_dummy2[] = {
  PROCESS_DUMMYIO2,
  3,
  CIODummyNotify::cfgIdDim(),
  10,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 10),
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  CIODummyNotify::cfgInterval(false),
  5000,
};

uint32_t g_cfg_process_virt2[] = {
  PROCESS_VIRTIO2,
  0,
};
