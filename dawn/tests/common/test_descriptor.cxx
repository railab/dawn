// dawn/tests/common/test_descriptor.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/descriptor.hxx"
#include "dawn/io/common.hxx"
#include "test_common.hxx"
#include "test_mockhandler.hxx"

using namespace dawn;

// Invalid magic

uint32_t g_bin_no_magic[] = {0, 0, 0, 0};

// Valid magic, but no objects

uint32_t g_bin_zero_size[] = {
  CDescriptor::DAWN_DESCRIPTOR_HDR, // Magic
  0,                                // Size
};

// Valid config with no additional config

uint32_t g_bin_valid_no_config[] = {
  // Header

  CDescriptor::DAWN_DESCRIPTOR_HDR, // Magic
  3,                                // Size

  // Object 0

  SObjectId::objectId(SObjectId::OBJTYPE_IO,
                      CIOCommon::IO_CLASS_VIRT,
                      SObjectId::DTYPE_UINT32,
                      0,
                      1),
  0,

  // Object 1

  SObjectId::objectId(SObjectId::OBJTYPE_IO,
                      CIOCommon::IO_CLASS_VIRT,
                      SObjectId::DTYPE_UINT32,
                      0,
                      2),
  0,

  // Object 2

  SObjectId::objectId(SObjectId::OBJTYPE_IO,
                      CIOCommon::IO_CLASS_VIRT,
                      SObjectId::DTYPE_UINT32,
                      0,
                      3),
  0,

  // Check sum

  CDescriptor::DAWN_DESCRIPTOR_FOOT,
  0xdeadbeef};

// Valid config with additional config

uint32_t g_bin_valid_config[] = {
  // Header

  CDescriptor::DAWN_DESCRIPTOR_HDR, // Magic
  3,                                // Size

  // Object 0

  SObjectId::objectId(SObjectId::OBJTYPE_IO,
                      CIOCommon::IO_CLASS_VIRT,
                      SObjectId::DTYPE_UINT32,
                      0,
                      1),
  1,
  0,

  // Object 1

  SObjectId::objectId(SObjectId::OBJTYPE_IO,
                      CIOCommon::IO_CLASS_VIRT,
                      SObjectId::DTYPE_UINT32,
                      0,
                      2),
  1,
  0,

  // Object 2

  SObjectId::objectId(SObjectId::OBJTYPE_IO,
                      CIOCommon::IO_CLASS_VIRT,
                      SObjectId::DTYPE_UINT32,
                      0,
                      3),
  0,

  // Check sum

  CDescriptor::DAWN_DESCRIPTOR_FOOT,
  0xdeadbeef};

// Allocobj_func_t mock

std::vector<CDescObject *> g_mock_allockobj_objects;

void mock_allocobj_func(const CHandler &obj, CDescObject &desc)
{
  g_mock_allockobj_objects.push_back(&desc);
}

//***************************************************************************
// Description: validate check sum
//***************************************************************************

static void test_descriptor_descriptor_bin_checksum()
{
  int ret;

  uint32_t invalid1[] = {
    CDescriptor::DAWN_DESCRIPTOR_HDR, // Magic
    1,                                // Size
    0,
    0,                                // Dummy config
                                      // No last byte
  };

  uint32_t invalid2[] = {
    CDescriptor::DAWN_DESCRIPTOR_HDR, // Magic
    1,                                // Size
    0,
    0,                                // Dummy config
    CDescriptor::DAWN_DESCRIPTOR_FOOT // Last byte
                                      // No sum byte reserved
  };

  uint32_t valid[] = {
    CDescriptor::DAWN_DESCRIPTOR_HDR,  // Magic
    1,                                 // Size
    0,
    0,                                 // Dummy config
    CDescriptor::DAWN_DESCRIPTOR_FOOT, // Last byte
    0                                  // Sum byte reserved
  };

  ret = CDescriptor::binCheckFill(invalid1, sizeof(invalid1));
  TEST_ASSERT_EQUAL(ret, -EINVAL);

  ret = CDescriptor::binValid(invalid1, sizeof(invalid1));
  TEST_ASSERT_EQUAL(ret, -EINVAL);

  ret = CDescriptor::binCheckFill(invalid2, sizeof(invalid2));
  TEST_ASSERT_EQUAL(ret, -EINVAL);

  ret = CDescriptor::binValid(invalid2, sizeof(invalid2));
  TEST_ASSERT_EQUAL(ret, -EINVAL);

  ret = CDescriptor::binCheckFill(valid, sizeof(valid));
  TEST_ASSERT_EQUAL(ret, OK);

  ret = CDescriptor::binValid(valid, sizeof(valid));
  TEST_ASSERT_EQUAL(ret, OK);
}

//***************************************************************************
// Description: validate descriptor in binary format
//***************************************************************************

static void test_descriptor_descriptor_bin_valid()
{
  CDescriptor desc;
  int ret;

  // Invalid config

  ret = desc.binValid(g_bin_no_magic, 0);
  TEST_ASSERT_EQUAL(ret, -EINVAL);

  ret = desc.binValid(g_bin_no_magic, sizeof(g_bin_no_magic));
  TEST_ASSERT_EQUAL(ret, -EINVAL);

  ret = desc.binValid(g_bin_zero_size, sizeof(g_bin_zero_size));
  TEST_ASSERT_EQUAL(ret, -EINVAL);

  // Fill check sum for valid configs

  ret = CDescriptor::binCheckFill(g_bin_valid_no_config, sizeof(g_bin_valid_no_config));
  TEST_ASSERT_EQUAL(ret, 0);

  ret = CDescriptor::binCheckFill(g_bin_valid_config, sizeof(g_bin_valid_config));
  TEST_ASSERT_EQUAL(ret, 0);

  // Valid configurations

  ret = desc.binValid(g_bin_valid_no_config, sizeof(g_bin_valid_no_config));
  TEST_ASSERT_EQUAL(ret, OK);

  ret = desc.binValid(g_bin_valid_config, sizeof(g_bin_valid_config));
  TEST_ASSERT_EQUAL(ret, OK);
}

//***************************************************************************
// Description: dump binary descriptor
//***************************************************************************

static void test_descriptor_descriptor_bin_dump()

{
  CDescriptor::binDump(g_bin_valid_no_config, sizeof(g_bin_valid_no_config));
}

//***************************************************************************
// Description: descriptor object allocation visits every object entry.
//***************************************************************************

static void test_descriptor_descriptor_bin_objalloc()
{
  CDescriptor desc;
  CMockHandler handler;
  int ret;

  ret = desc.loadBin(g_bin_valid_no_config, sizeof(g_bin_valid_no_config));
  TEST_ASSERT_EQUAL(ret, OK);

  // Alloc object

  g_mock_allockobj_objects.clear();
  desc.alloc_objects(handler, mock_allocobj_func);
  TEST_ASSERT_EQUAL(g_mock_allockobj_objects.size(), 3);
}

//***************************************************************************
// Description: descriptor reset clears loaded binary state.
//***************************************************************************

static void test_descriptor_reset()
{
  CDescriptor desc;
  uint32_t valid[] = {
    CDescriptor::DAWN_DESCRIPTOR_HDR,
    1,
    SObjectId::objectId(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_VIRT, SObjectId::DTYPE_UINT32, 0, 1),
    0,
    CDescriptor::DAWN_DESCRIPTOR_FOOT,
    0xdeadbeef,
  };
  int ret;

  ret = CDescriptor::binCheckFill(valid, sizeof(valid));
  TEST_ASSERT_EQUAL(OK, ret);

  ret = desc.loadBin(valid, sizeof(valid));
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT(desc.getBin() != nullptr);
  TEST_ASSERT_EQUAL(sizeof(valid), desc.getBinLen());

  desc.reset();
  TEST_ASSERT_EQUAL(nullptr, desc.getBin());
  TEST_ASSERT_EQUAL(0u, desc.getBinLen());

  ret = desc.loadBin(valid, sizeof(valid));
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT(desc.getBin() != nullptr);
}

//***************************************************************************
// Description: no-idle-quit metadata is cleared by subsequent loads.
//***************************************************************************

static void test_descriptor_no_idle_quit_metadata_resets()
{
  CDescriptor desc;
  uint32_t withNoIdle[] = {
    CDescriptor::DAWN_DESCRIPTOR_HDR,
    1,
    CDescriptor::objectId(1),
    1,
    CDescriptor::cfgIdNoIdleQuit(),
    1,
    CDescriptor::DAWN_DESCRIPTOR_FOOT,
    0xdeadbeef,
  };
  uint32_t withoutMeta[] = {
    CDescriptor::DAWN_DESCRIPTOR_HDR,
    1,
    SObjectId::objectId(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_VIRT, SObjectId::DTYPE_UINT32, 0, 1),
    0,
    CDescriptor::DAWN_DESCRIPTOR_FOOT,
    0xdeadbeef,
  };
  int ret;

  ret = CDescriptor::binCheckFill(withNoIdle, sizeof(withNoIdle));
  TEST_ASSERT_EQUAL(OK, ret);
  ret = desc.loadBin(withNoIdle, sizeof(withNoIdle));
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_TRUE(desc.getNoIdleQuit());

  ret = CDescriptor::binCheckFill(withoutMeta, sizeof(withoutMeta));
  TEST_ASSERT_EQUAL(OK, ret);
  ret = desc.loadBin(withoutMeta, sizeof(withoutMeta));
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_FALSE(desc.getNoIdleQuit());
}

extern "C"
{
  int test_common_descriptor()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_descriptor_descriptor_bin_checksum);
    DAWN_RUN_TEST(test_descriptor_descriptor_bin_valid);
    DAWN_RUN_TEST(test_descriptor_descriptor_bin_dump);
    DAWN_RUN_TEST(test_descriptor_descriptor_bin_objalloc);
    DAWN_RUN_TEST(test_descriptor_reset);
    DAWN_RUN_TEST(test_descriptor_no_idle_quit_metadata_resets);

    return UNITY_END();
  }
}
