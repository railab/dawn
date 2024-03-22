// dawn/tests/proto/test_common.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "test_common.hxx"

#include "dawn/proto/common.hxx"
#include "test_iomockcommon.hxx"
#include "test_protomockcommon.hxx"

using namespace dawn;

static constexpr auto ALLOCITEM_ID_1 = (SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 0));
static constexpr auto ALLOCITEM_ID_2 = (SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 2));
static constexpr auto ALLOCITEM_ID_3 = (SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 3));
static constexpr auto ALLOCITEM_ID_4 = (SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 4));

static uint32_t g_cfg_proto[] = {
  SObjectId::objectId(SObjectId::OBJTYPE_PROTO,
                      CProtoCommon::PROTO_CLASS_USER,
                      SObjectId::DTYPE_ANY,
                      0,
                      1),
  3,

  // Config item 1 - alloc object

  SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROTO,
                        CProtoCommon::PROTO_CLASS_USER,
                        SObjectId::DTYPE_ANY,
                        false,
                        1,
                        CProtoMockCommon::PROTO_MOCK_CFG_IOBIND),
  ALLOCITEM_ID_1,

  // Config item 2 - alloc object

  SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROTO,
                        CProtoCommon::PROTO_CLASS_USER,
                        SObjectId::DTYPE_ANY,
                        false,
                        1,
                        CProtoMockCommon::PROTO_MOCK_CFG_IOBIND),
  ALLOCITEM_ID_2,

  // Config item 3 - alloc object

  SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROTO,
                        CProtoCommon::PROTO_CLASS_USER,
                        SObjectId::DTYPE_ANY,
                        false,
                        1,
                        CProtoMockCommon::PROTO_MOCK_CFG_IOBIND),
  ALLOCITEM_ID_3,
};

static uint32_t g_cfg1[] = {
  SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 0),
  0,
};

static uint32_t g_cfg2[] = {
  SObjectId::objectId(SObjectId::OBJTYPE_PROTO,
                      CProtoCommon::PROTO_CLASS_USER,
                      SObjectId::DTYPE_ANY,
                      0,
                      1),
  0,
};

static uint32_t g_cfg4[] = {
  SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 2),
  0,
};

//***************************************************************************
// Description: CProtoCommon tracks configured IO bindings and map updates.
//***************************************************************************

static void test_proto_common_init()
{
  CDescObject desc(g_cfg_proto);
  const std::map<SObjectId::ObjectId, CIOCommon *> *iomap;
  CProtoMockCommon proto(desc);
  CIOCommon *io = nullptr;
  CDescObject desc1(g_cfg1);
  CIOMockCommon io1(desc1);
  CDescObject desc2(g_cfg2);
  CProtoMockCommon proto1(desc2);
  CDescObject desc4(g_cfg4);
  CIOMockCommon io4(desc4);

  // Check IO map

  iomap = &proto.getIOMap();

  TEST_ASSERT_TRUE(iomap->contains(ALLOCITEM_ID_1));
  TEST_ASSERT_NULL(iomap->at(ALLOCITEM_ID_1));
  io = proto.getIO(ALLOCITEM_ID_1);
  TEST_ASSERT_NULL(io);

  // No item

  TEST_ASSERT_FALSE(iomap->contains(ALLOCITEM_ID_4));

  // Modify maps

  proto.setObjectMapItem(ALLOCITEM_ID_1, &io1);
  io = proto.getIO(ALLOCITEM_ID_1);
  TEST_ASSERT_EQUAL(io, &io1);

  // Set not existing element - create new

  proto.setObjectMapItem(ALLOCITEM_ID_4, &io4);
  io = proto.getIO(ALLOCITEM_ID_4);
  TEST_ASSERT_EQUAL(io, &io4);
}

extern "C"
{
  int test_proto_common()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_proto_common_init);

    return UNITY_END();
  }
}
