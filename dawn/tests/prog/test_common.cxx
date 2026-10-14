// dawn/tests/prog/test_common.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "test_common.hxx"
#include "dawn/io/ddata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/common.hxx"
#include "test_iomockcommon.hxx"
#include "test_progmockcommon.hxx"

using namespace dawn;

// Expose the protected helpers

class CProgCommonHelpers : public CProgMockCommon
{
public:
  using CProgMockCommon::CProgMockCommon;
  using CProgCommon::contiguousCount;
  using CProgCommon::prepareWritableTarget;
};

static constexpr auto ALLOCITEM_ID_1 = (SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 0));
static constexpr auto ALLOCITEM_ID_2 = (SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 2));
static constexpr auto ALLOCITEM_ID_3 = (SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 3));
static constexpr auto ALLOCITEM_ID_4 = (SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 4));

static uint32_t g_cfg_prog[] = {
  SObjectId::objectId(SObjectId::OBJTYPE_PROG,
                      CProgCommon::PROG_CLASS_USER,
                      SObjectId::DTYPE_ANY,
                      0,
                      1),
  3,

  // Config item 1 - alloc object

  SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                        CProgCommon::PROG_CLASS_USER,
                        SObjectId::DTYPE_ANY,
                        false,
                        1,
                        CProgMockCommon::PROG_MOCK_CFG_IOBIND),
  ALLOCITEM_ID_1,

  // Config item 2 - alloc object

  SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                        CProgCommon::PROG_CLASS_USER,
                        SObjectId::DTYPE_ANY,
                        false,
                        1,
                        CProgMockCommon::PROG_MOCK_CFG_IOBIND),
  ALLOCITEM_ID_2,

  // Config item 3 - alloc object

  SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                        CProgCommon::PROG_CLASS_USER,
                        SObjectId::DTYPE_ANY,
                        false,
                        1,
                        CProgMockCommon::PROG_MOCK_CFG_IOBIND),
  ALLOCITEM_ID_3,
};

static uint32_t g_cfg1[] = {ALLOCITEM_ID_1, 0};

static uint32_t g_cfg2[] = {
  ALLOCITEM_ID_2,
  0,
};

static uint32_t g_cfg4[] = {ALLOCITEM_ID_4, 0};

//***************************************************************************
// Description: CProgCommon tracks configured IO bindings and map updates.
//***************************************************************************

static void test_prog_common_init()
{
  CDescObject desc(g_cfg_prog);
  const std::map<SObjectId::ObjectId, CIOCommon *> *iomap;
  CProgMockCommon prog(desc);
  CIOCommon *io = nullptr;
  CDescObject desc1(g_cfg1);
  CIOMockCommon io1(desc1);
  CDescObject desc2(g_cfg2);
  CIOMockCommon io2(desc2);
  CDescObject desc4(g_cfg4);
  CIOMockCommon io4(desc4);

  // Check IO map

  iomap = &prog.getIOMap();

  TEST_ASSERT_TRUE(iomap->contains(ALLOCITEM_ID_1));
  TEST_ASSERT_NULL(iomap->at(ALLOCITEM_ID_1));
  io = prog.getIO(ALLOCITEM_ID_1);
  TEST_ASSERT_NULL(io);

  // No item

  TEST_ASSERT_FALSE(iomap->contains(ALLOCITEM_ID_4));

  // Modify maps

  prog.setObjectMapItem(ALLOCITEM_ID_1, &io1);
  io = prog.getIO(ALLOCITEM_ID_1);
  TEST_ASSERT_EQUAL(io, &io1);

  // Set not existing element - create new

  prog.setObjectMapItem(ALLOCITEM_ID_4, &io4);
  io = prog.getIO(ALLOCITEM_ID_4);
  TEST_ASSERT_EQUAL(io, &io4);
}

//***************************************************************************
// Description: contiguousCount() spans batches only when they are packed.
//***************************************************************************

static void test_prog_common_contiguous_count()
{
  io_ddata_t one(2, 3, 1);
  io_ddata_t packed(2, 3, 4);
  io_ddata_t stamped(2, 3, 4, 0, true);

  TEST_ASSERT_EQUAL(3, CProgCommonHelpers::contiguousCount(&one, 1, 3));
  TEST_ASSERT_EQUAL(12, CProgCommonHelpers::contiguousCount(&packed, 4, 3));
  TEST_ASSERT_EQUAL(0, CProgCommonHelpers::contiguousCount(&packed, 4, 2));
  TEST_ASSERT_EQUAL(0, CProgCommonHelpers::contiguousCount(&stamped, 4, 3));
  TEST_ASSERT_EQUAL(3, CProgCommonHelpers::contiguousCount(&stamped, 1, 3));
}

#ifdef CONFIG_DAWN_IO_VIRT
static uint32_t g_cfg_virt[] = {
  CIOVirt::objectId(SObjectId::DTYPE_INT16, false, 160),
  0,
};

//***************************************************************************
// Description: prepareWritableTarget() shapes a deferred virt with the batch
// and rejects an already shaped one with a different batch.
//***************************************************************************

static void test_prog_common_prepare_batched_target()
{
  CDescObject desc(g_cfg_prog);
  CProgCommonHelpers prog(desc);
  CDescObject vdesc(g_cfg_virt);
  CIOVirt virt(vdesc);

  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, prog.prepareWritableTarget(&virt, 1, true, 4));
  TEST_ASSERT_EQUAL(1, virt.getDataDim());
#  ifdef CONFIG_DAWN_IO_NOTIFY
  TEST_ASSERT_EQUAL(4, virt.getNotifyBatch());
  TEST_ASSERT_EQUAL(-EINVAL, prog.prepareWritableTarget(&virt, 1, true, 2));
#  endif
  TEST_ASSERT_EQUAL(OK, prog.prepareWritableTarget(&virt, 1, true, 4));
}
#endif

extern "C"
{
  int test_prog_common()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_prog_common_init);
    DAWN_RUN_TEST(test_prog_common_contiguous_count);
#ifdef CONFIG_DAWN_IO_VIRT
    DAWN_RUN_TEST(test_prog_common_prepare_batched_target);
#endif

    return UNITY_END();
  }
}
