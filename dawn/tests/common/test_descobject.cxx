// dawn/tests/common/test_descobject.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/descobject.hxx"
#include "dawn/io/common.hxx"
#include "test_common.hxx"

using namespace dawn;

// Valid config with no additional config

uint32_t g_bin_valid[] = {
  SObjectId::objectId(SObjectId::OBJTYPE_IO,
                      CIOCommon::IO_CLASS_VIRT,
                      SObjectId::DTYPE_UINT32,
                      0,
                      1),
  4,

  SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                        CIOCommon::IO_CLASS_VIRT,
                        SObjectId::DTYPE_ANY,
                        false,
                        0,
                        1),

  SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                        CIOCommon::IO_CLASS_VIRT,
                        SObjectId::DTYPE_ANY,
                        false,
                        1,
                        1),
  0x0000000f,

  SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                        CIOCommon::IO_CLASS_VIRT,
                        SObjectId::DTYPE_ANY,
                        false,
                        2,
                        1),
  0x000000ff,
  0x00000fff,

  SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                        CIOCommon::IO_CLASS_VIRT,
                        SObjectId::DTYPE_ANY,
                        false,
                        3,
                        1),
  0x0000ffff,
  0x000fffff,
  0x00ffffff,
};

//***************************************************************************
// Description: getObjectId() returns the descriptor's parsed object id
// fields (type, cls, dtype, priv).
//***************************************************************************

static void test_descobject_object_id_fields()
{
  CDescObject obj(g_bin_valid);
  SObjectId::UObjectId &id = obj.getObjectId();

  TEST_ASSERT_EQUAL(SObjectId::OBJTYPE_IO, id.s.type);
  TEST_ASSERT_EQUAL(CIOCommon::IO_CLASS_VIRT, id.s.cls);
  TEST_ASSERT_EQUAL(SObjectId::DTYPE_UINT32, id.s.dtype);
  TEST_ASSERT_EQUAL(1, id.s.priv);
}

//***************************************************************************
// Description: getObjectCls/Type/Dtype mirror the parsed object id.
//***************************************************************************

static void test_descobject_accessor_helpers()
{
  CDescObject obj(g_bin_valid);

  TEST_ASSERT_EQUAL(CIOCommon::IO_CLASS_VIRT, obj.getObjectCls());
  TEST_ASSERT_EQUAL(SObjectId::OBJTYPE_IO, obj.getObjectType());
  TEST_ASSERT_EQUAL(SObjectId::DTYPE_UINT32, obj.getObjectDtype());
}

//***************************************************************************
// Description: getCfg / getSize / getSizeBytes return the buffer pointer,
// the cfg-item count, and the total size in bytes.
//***************************************************************************

static void test_descobject_size_metadata()
{
  CDescObject obj(g_bin_valid);

  TEST_ASSERT_EQUAL(g_bin_valid, obj.getCfg());
  TEST_ASSERT_EQUAL(4, obj.getSize());
  TEST_ASSERT_EQUAL(48, obj.getSizeBytes());
}

//***************************************************************************
// Description: objectCfgItemAtOffset() returns the cfg-item at the given
// word offset; the first data word is reachable through item->data[0].
//***************************************************************************

static void test_descobject_cfg_item_at_offset()
{
  CDescObject obj(g_bin_valid);
  SObjectCfg::SObjectCfgItem *item;

  item = obj.objectCfgItemAtOffset(1);
  TEST_ASSERT_EQUAL(0x0000000f, item->data[0]);

  item = obj.objectCfgItemAtOffset(6);
  TEST_ASSERT_EQUAL(0x0000ffff, item->data[0]);
}

//***************************************************************************
// Description: objectCfgItemId() looks up a cfg item by its config id and
// returns its first data word.
//***************************************************************************

static void test_descobject_cfg_item_by_id()
{
  CDescObject obj(g_bin_valid);
  SObjectCfg::SObjectCfgItem *item;

  item = obj.objectCfgItemId(SObjectCfg::objectCfg(
    SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_VIRT, SObjectId::DTYPE_ANY, false, 2, 1));
  TEST_ASSERT_EQUAL(0x000000ff, item->data[0]);
}

//***************************************************************************
// Description: getAtOffset() returns the raw uint32 stored at the given
// offset within the descriptor.
//***************************************************************************

static void test_descobject_get_at_offset()
{
  CDescObject obj(g_bin_valid);

  TEST_ASSERT_EQUAL(4, obj.getAtOffset(1));
  TEST_ASSERT_EQUAL(0x00000fff, obj.getAtOffset(7));
}

extern "C"
{
  int test_common_descobject()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_descobject_object_id_fields);
    DAWN_RUN_TEST(test_descobject_accessor_helpers);
    DAWN_RUN_TEST(test_descobject_size_metadata);
    DAWN_RUN_TEST(test_descobject_cfg_item_at_offset);
    DAWN_RUN_TEST(test_descobject_cfg_item_by_id);
    DAWN_RUN_TEST(test_descobject_get_at_offset);

    return UNITY_END();
  }
}
