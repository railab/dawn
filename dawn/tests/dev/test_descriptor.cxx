// dawn/tests/dev/test_descriptor.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>

#include "dawn/dev/descriptor.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: check descriptor manager implementation
//***************************************************************************

static void test_dev_descriptor_init()
{
  CDevDescriptor *inst1 = CDevDescriptor::getInst();
  CDevDescriptor *inst2 = CDevDescriptor::getInst();
  CDevDescriptor::SDescriptorReg reg;
  static uint8_t desc0;
#if CONFIG_DAWN_DESC_SLOTS > 1
  static uint8_t desc1;
#endif
  int ret;

  // Singleton

  TEST_ASSERT(inst1 == inst2);

  // Default zeros

  ret = inst1->getDescriptor(0, reg);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(reg.ptr, 0);
  TEST_ASSERT_EQUAL(reg.len, 0);

#if CONFIG_DAWN_DESC_SLOTS > 1
  ret = inst1->getDescriptor(1, reg);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(reg.ptr, 0);
  TEST_ASSERT_EQUAL(reg.len, 0);
#endif

  ret = inst1->getDescriptor(CDevDescriptor::MAX_DESCRIPTORS + 1, reg);
  TEST_ASSERT_EQUAL(-ENOMEM, ret);

  // Register descriptors

  reg.ptr = &desc0;
  reg.len = 2;
  ret = inst1->regDescriptor(0, reg);
  TEST_ASSERT_EQUAL(OK, ret);

#if CONFIG_DAWN_DESC_SLOTS > 1
  reg.ptr = &desc1;
  reg.len = 3;
  ret = inst1->regDescriptor(1, reg);
  TEST_ASSERT_EQUAL(OK, ret);
#endif

  // Should be registered

  ret = inst1->getDescriptor(0, reg);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL_PTR(&desc0, reg.ptr);
  TEST_ASSERT_EQUAL(reg.len, 2);

#if CONFIG_DAWN_DESC_SLOTS > 1
  ret = inst1->getDescriptor(1, reg);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL_PTR(&desc1, reg.ptr);
  TEST_ASSERT_EQUAL(reg.len, 3);
#endif

  ret = inst1->regDescriptor(CDevDescriptor::MAX_DESCRIPTORS + 1, reg);
  TEST_ASSERT_EQUAL(-ENOMEM, ret);

  // Clean up

  CDevDescriptor::destroy();
}

#if CONFIG_DAWN_DESC_SLOTS > 1

//***************************************************************************
// Description: descriptor slot writes allocate storage and preserve bytes.
//***************************************************************************

static void test_dev_descriptor_slot_write()
{
  CDevDescriptor *inst;
  CDevDescriptor::SDescriptorReg reg;
  uint8_t payload[4] = {0x11, 0x22, 0x33, 0x44};
  int ret;

  CDevDescriptor::destroy();
  inst = CDevDescriptor::getInst();

  ret = inst->writeSlotData(1, payload, 0, sizeof(payload));
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(sizeof(payload), inst->getSlotWritten(1));

  ret = inst->getDescriptor(1, reg);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT(reg.ptr != nullptr);
  TEST_ASSERT_EQUAL(sizeof(payload), reg.len);
  TEST_ASSERT_EQUAL(0x11, static_cast<uint8_t *>(reg.ptr)[0]);
  TEST_ASSERT_EQUAL(0x22, static_cast<uint8_t *>(reg.ptr)[1]);
  TEST_ASSERT_EQUAL(0x33, static_cast<uint8_t *>(reg.ptr)[2]);
  TEST_ASSERT_EQUAL(0x44, static_cast<uint8_t *>(reg.ptr)[3]);

  CDevDescriptor::destroy();
}

//***************************************************************************
// Description: descriptor slot writes reject payloads past slot capacity.
//***************************************************************************

static void test_dev_descriptor_slot_write_overflow()
{
  CDevDescriptor *inst;
  uint8_t payload[8] = {0};
  int ret;

  CDevDescriptor::destroy();
  inst = CDevDescriptor::getInst();

  ret = inst->writeSlotData(1, payload, CDevDescriptor::SLOT_SIZE - 4, sizeof(payload));
  TEST_ASSERT_EQUAL(-ENOSPC, ret);

  CDevDescriptor::destroy();
}

//***************************************************************************
// Description: descriptor slot reset clears stored data and length.
//***************************************************************************

static void test_dev_descriptor_slot_reset()
{
  CDevDescriptor *inst;
  CDevDescriptor::SDescriptorReg reg;
  uint8_t payload[3] = {1, 2, 3};
  int ret;

  CDevDescriptor::destroy();
  inst = CDevDescriptor::getInst();

  ret = inst->writeSlotData(1, payload, 0, sizeof(payload));
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(sizeof(payload), inst->getSlotWritten(1));

  ret = inst->resetSlot(1);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(0u, inst->getSlotWritten(1));

  ret = inst->getDescriptor(1, reg);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(nullptr, reg.ptr);
  TEST_ASSERT_EQUAL(0u, reg.len);

  CDevDescriptor::destroy();
}

//***************************************************************************
// Description: descriptor slot written length tracks sparse writes.
//***************************************************************************

static void test_dev_descriptor_slot_written()
{
  CDevDescriptor *inst;
  uint8_t payload[2] = {0xaa, 0xbb};
  int ret;

  CDevDescriptor::destroy();
  inst = CDevDescriptor::getInst();

  TEST_ASSERT_EQUAL(0u, inst->getSlotWritten(1));

  ret = inst->writeSlotData(1, payload, 3, sizeof(payload));
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(5u, inst->getSlotWritten(1));

  ret = inst->writeSlotData(1, payload, 0, 1);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(1u, inst->getSlotWritten(1));

  CDevDescriptor::destroy();
}

//***************************************************************************
// Description: descriptor slot 0 rejects runtime writes.
//***************************************************************************

static void test_dev_descriptor_slot0_readonly()
{
  CDevDescriptor *inst;
  uint8_t payload[1] = {0xff};
  int ret;

  CDevDescriptor::destroy();
  inst = CDevDescriptor::getInst();

  ret = inst->writeSlotData(0, payload, 0, sizeof(payload));
  TEST_ASSERT_EQUAL(-EINVAL, ret);

  CDevDescriptor::destroy();
}
#endif

extern "C"
{
  int test_dev_descriptor()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_dev_descriptor_init);
#if CONFIG_DAWN_DESC_SLOTS > 1
    DAWN_RUN_TEST(test_dev_descriptor_slot_write);
    DAWN_RUN_TEST(test_dev_descriptor_slot_write_overflow);
    DAWN_RUN_TEST(test_dev_descriptor_slot_reset);
    DAWN_RUN_TEST(test_dev_descriptor_slot_written);
    DAWN_RUN_TEST(test_dev_descriptor_slot0_readonly);
#endif

    return UNITY_END();
  }
}
