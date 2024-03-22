// dawn/tests/io/test_descselector.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>

#include "dawn/dev/descriptor.hxx"
#include "dawn/dev/descswitch.hxx"
#include "dawn/io/ddata.hxx"
#include "dawn/io/descselector.hxx"
#include "test_common.hxx"

using namespace dawn;

static uint32_t g_cfg_descselector0[] = {
  CIODescSelector::objectId(0),
  0,
};

static void build_valid_bin(uint32_t *bin, size_t len, uint16_t inst)
{
  int ret;

  bin[0] = CDescriptor::DAWN_DESCRIPTOR_HDR;
  bin[1] = 1;
  bin[2] = SObjectId::objectId(
    SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_VIRT, SObjectId::DTYPE_UINT32, 0, inst);
  bin[3] = 0;
  bin[4] = CDescriptor::DAWN_DESCRIPTOR_FOOT;
  bin[5] = 0xdeadbeef;

  ret = CDescriptor::binCheckFill(bin, len);
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: descselector configures and initializes with default state.
//***************************************************************************

static void test_io_descselector_init()
{
  CDescObject desc(g_cfg_descselector0);
  CIODescSelector io(desc);
  int ret;

  ret = io.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  ret = io.init();
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: descselector reads the currently active descriptor slot.
//***************************************************************************

static void test_io_descselector_read_active_slot()
{
  CDescObject desc(g_cfg_descselector0);
  CIODescSelector io(desc);
  io_ddata_t *data;
  int ret;

  CDescSwitch::clear();
  CDescSwitch::setActiveSlot(0);

  ret = io.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  ret = io.init();
  TEST_ASSERT_EQUAL(OK, ret);

  data = io.ddata_alloc(1);
  TEST_ASSERT(data != nullptr);

  ret = io.getData(*data, 1);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(0u, *reinterpret_cast<uint32_t *>(data->getDataPtr()));

  CDescSwitch::setActiveSlot(2);
  ret = io.getData(*data, 1);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(2u, *reinterpret_cast<uint32_t *>(data->getDataPtr()));

  free(data);
}

//***************************************************************************
// Description: descselector rejects slot numbers outside configured range.
//***************************************************************************

static void test_io_descselector_write_invalid_slot()
{
  CDescObject desc(g_cfg_descselector0);
  CIODescSelector io(desc);
  io_ddata_t *data;
  int ret;

  CDescSwitch::clear();
  CDescSwitch::setActiveSlot(0);

  ret = io.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  data = io.ddata_alloc(1);
  TEST_ASSERT(data != nullptr);

  *reinterpret_cast<uint32_t *>(data->getDataPtr()) = CONFIG_DAWN_DESC_SLOTS;
  ret = io.setData(*data);
  TEST_ASSERT_EQUAL(-EINVAL, ret);

  free(data);
}

//***************************************************************************
// Description: selecting the active slot is a no-op.
//***************************************************************************

static void test_io_descselector_write_same_slot()
{
  CDescObject desc(g_cfg_descselector0);
  CIODescSelector io(desc);
  io_ddata_t *data;
  int ret;

  CDescSwitch::clear();
  CDescSwitch::setActiveSlot(0);

  ret = io.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  data = io.ddata_alloc(1);
  TEST_ASSERT(data != nullptr);

  *reinterpret_cast<uint32_t *>(data->getDataPtr()) = 0;
  ret = io.setData(*data);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(false, CDescSwitch::isSwitchRequested());

  free(data);
}

#if CONFIG_DAWN_DESC_SLOTS > 1

//***************************************************************************
// Description: selecting an empty descriptor slot returns -ENODATA.
//***************************************************************************

static void test_io_descselector_write_empty_slot()
{
  CDescObject desc(g_cfg_descselector0);
  CIODescSelector io(desc);
  io_ddata_t *data;
  int ret;

  CDescSwitch::clear();
  CDescSwitch::setActiveSlot(0);
  CDevDescriptor::destroy();
  CDevDescriptor::getInst();

  ret = io.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  data = io.ddata_alloc(1);
  TEST_ASSERT(data != nullptr);

  *reinterpret_cast<uint32_t *>(data->getDataPtr()) = 1;
  ret = io.setData(*data);
  TEST_ASSERT_EQUAL(-ENODATA, ret);
  TEST_ASSERT_EQUAL(false, CDescSwitch::isSwitchRequested());

  free(data);
  CDevDescriptor::destroy();
}

//***************************************************************************
// Description: selecting a descriptor with bad CRC returns -EBADMSG.
//***************************************************************************

static void test_io_descselector_write_bad_crc()
{
  CDescObject desc(g_cfg_descselector0);
  CIODescSelector io(desc);
  CDevDescriptor::SDescriptorReg reg;
  io_ddata_t *data;
  uint32_t bin[6];
  int ret;

  CDescSwitch::clear();
  CDescSwitch::setActiveSlot(0);
  CDevDescriptor::destroy();

  build_valid_bin(bin, sizeof(bin), 11);
  bin[5] ^= 0x1u;

  reg.ptr = bin;
  reg.len = sizeof(bin);
  ret = CDevDescriptor::getInst()->regDescriptor(1, reg);
  TEST_ASSERT_EQUAL(OK, ret);

  ret = io.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  data = io.ddata_alloc(1);
  TEST_ASSERT(data != nullptr);
  *reinterpret_cast<uint32_t *>(data->getDataPtr()) = 1;

  ret = io.setData(*data);
  TEST_ASSERT_EQUAL(-EBADMSG, ret);
  TEST_ASSERT_EQUAL(false, CDescSwitch::isSwitchRequested());

  free(data);
  CDevDescriptor::destroy();
}

//***************************************************************************
// Description: selecting a valid descriptor requests a slot switch.
//***************************************************************************

static void test_io_descselector_write_valid()
{
  CDescObject desc(g_cfg_descselector0);
  CIODescSelector io(desc);
  CDevDescriptor::SDescriptorReg reg;
  io_ddata_t *data;
  uint32_t bin[6];
  int ret;

  CDescSwitch::clear();
  CDescSwitch::setActiveSlot(0);
  CDevDescriptor::destroy();

  build_valid_bin(bin, sizeof(bin), 12);
  reg.ptr = bin;
  reg.len = sizeof(bin);
  ret = CDevDescriptor::getInst()->regDescriptor(1, reg);
  TEST_ASSERT_EQUAL(OK, ret);

  ret = io.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  data = io.ddata_alloc(1);
  TEST_ASSERT(data != nullptr);
  *reinterpret_cast<uint32_t *>(data->getDataPtr()) = 1;

  ret = io.setData(*data);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(true, CDescSwitch::isSwitchRequested());
  TEST_ASSERT_EQUAL(1u, CDescSwitch::getSwitchSlot());

  free(data);
  CDevDescriptor::destroy();
}

//***************************************************************************
// Description: selecting slot 0 supports rollback to the boot descriptor.
//***************************************************************************

static void test_io_descselector_write_slot0_rollback()
{
  CDescObject desc(g_cfg_descselector0);
  CIODescSelector io(desc);
  CDevDescriptor::SDescriptorReg reg;
  io_ddata_t *data;
  uint32_t bin0[6];
  int ret;

  CDescSwitch::clear();
  CDescSwitch::setActiveSlot(1);
  CDevDescriptor::destroy();

  build_valid_bin(bin0, sizeof(bin0), 13);
  reg.ptr = bin0;
  reg.len = sizeof(bin0);
  ret = CDevDescriptor::getInst()->regDescriptor(0, reg);
  TEST_ASSERT_EQUAL(OK, ret);

  ret = io.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  data = io.ddata_alloc(1);
  TEST_ASSERT(data != nullptr);
  *reinterpret_cast<uint32_t *>(data->getDataPtr()) = 0;

  ret = io.setData(*data);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(true, CDescSwitch::isSwitchRequested());
  TEST_ASSERT_EQUAL(0u, CDescSwitch::getSwitchSlot());

  free(data);
  CDevDescriptor::destroy();
}
#endif

extern "C"
{
  int test_io_descselector()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_descselector_init);
    DAWN_RUN_TEST(test_io_descselector_read_active_slot);
    DAWN_RUN_TEST(test_io_descselector_write_invalid_slot);
    DAWN_RUN_TEST(test_io_descselector_write_same_slot);
#if CONFIG_DAWN_DESC_SLOTS > 1
    DAWN_RUN_TEST(test_io_descselector_write_empty_slot);
    DAWN_RUN_TEST(test_io_descselector_write_bad_crc);
    DAWN_RUN_TEST(test_io_descselector_write_valid);
    DAWN_RUN_TEST(test_io_descselector_write_slot0_rollback);
#endif

    return UNITY_END();
  }
}
