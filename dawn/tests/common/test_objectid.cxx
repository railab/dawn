// dawn/tests/common/test_objectid.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/objectid.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: get data type size
//***************************************************************************

static void test_common_objectid_dtype()
{
#ifdef CONFIG_DAWN_DTYPE_UINT32
  TEST_ASSERT_EQUAL(SObjectId::getDtypeSize_(SObjectId::DTYPE_UINT32), 4);
#else
  TEST_ASSERT_EQUAL(SObjectId::getDtypeSize_(SObjectId::DTYPE_UINT32), -1);
#endif

#ifdef CONFIG_DAWN_DTYPE_INT32
  TEST_ASSERT_EQUAL(SObjectId::getDtypeSize_(SObjectId::DTYPE_INT32), 4);
#else
  TEST_ASSERT_EQUAL(SObjectId::getDtypeSize_(SObjectId::DTYPE_INT32), -1);
#endif
}

//***************************************************************************
// Description: get object id
//***************************************************************************

static void test_common_objectid_objectid()
{
  SObjectId::ObjectId cfg1 = SObjectId::objectId(1, 4, 3, 1, 2);

  TEST_ASSERT_EQUAL(SObjectId::objectIdGetType(cfg1), 1);
  TEST_ASSERT_EQUAL(SObjectId::objectIdGetCls(cfg1), 4);
  TEST_ASSERT_EQUAL(SObjectId::objectIdGetDtype(cfg1), 3);
  TEST_ASSERT_EQUAL(SObjectId::objectIdGetFlags(cfg1), 1);
  TEST_ASSERT_EQUAL(SObjectId::objectIdGetId(cfg1), 2);
}

//***************************************************************************
// Description: get object type from from binary
//***************************************************************************

static void test_common_objectid_object_type()
{
  uint32_t bin_inval[] = {0x00000000};
  uint32_t bin_io[] = {SObjectId::objectId(SObjectId::OBJTYPE_IO, 0, 0, 0, 0)};
  uint32_t bin_proto[] = {SObjectId::objectId(SObjectId::OBJTYPE_PROTO, 0, 0, 0, 0)};
  uint32_t bin_prog[] = {SObjectId::objectId(SObjectId::OBJTYPE_PROG, 0, 0, 0, 0)};

  TEST_ASSERT(SObjectId::objectIsIO(reinterpret_cast<SObjectId::UObjectId &>(bin_inval)) == false);
  TEST_ASSERT(SObjectId::objectIsProto(reinterpret_cast<SObjectId::UObjectId &>(bin_inval)) ==
              false);
  TEST_ASSERT(SObjectId::objectIsProg(reinterpret_cast<SObjectId::UObjectId &>(bin_inval)) ==
              false);

  TEST_ASSERT(SObjectId::objectIsIO(reinterpret_cast<SObjectId::UObjectId &>(bin_io)) == true);
  TEST_ASSERT(SObjectId::objectIsProto(reinterpret_cast<SObjectId::UObjectId &>(bin_proto)) ==
              true);
  TEST_ASSERT(SObjectId::objectIsProg(reinterpret_cast<SObjectId::UObjectId &>(bin_prog)) == true);
}

//***************************************************************************
// System objects use OBJTYPE_ANY with cls != 0; cls 0 is descriptor metadata.
//***************************************************************************

static void test_common_objectid_system()
{
  uint32_t bin_meta[] = {SObjectId::objectId(SObjectId::OBJTYPE_ANY, 0, 0, 0, 0)};
  uint32_t bin_sys[] = {SObjectId::objectId(SObjectId::OBJTYPE_ANY, 1, 0, 0, 0)};
  uint32_t bin_io[] = {SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 0, 0, 0)};

  // Metadata (cls 0) is not a system object.
  TEST_ASSERT(SObjectId::objectIsSystem(reinterpret_cast<SObjectId::UObjectId &>(bin_meta)) ==
              false);

  // OBJTYPE_ANY with cls != 0 is a system object.
  TEST_ASSERT(SObjectId::objectIsSystem(reinterpret_cast<SObjectId::UObjectId &>(bin_sys)) == true);

  // An IO object is never a system object.
  TEST_ASSERT(SObjectId::objectIsSystem(reinterpret_cast<SObjectId::UObjectId &>(bin_io)) == false);
  TEST_ASSERT(SObjectId::objectIsIO(reinterpret_cast<SObjectId::UObjectId &>(bin_sys)) == false);
}

extern "C"
{
  int test_common_objectid()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_common_objectid_dtype);
    DAWN_RUN_TEST(test_common_objectid_objectid);
    DAWN_RUN_TEST(test_common_objectid_object_type);
    DAWN_RUN_TEST(test_common_objectid_system);

    return UNITY_END();
  }
}
