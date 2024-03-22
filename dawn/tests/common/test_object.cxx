// dawn/tests/common/test_object.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/object.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto OBJECT_ID = SObjectId::objectId(3, 3, SObjectId::DTYPE_INT32, 0, 0xee);
static constexpr auto OBJCFG_ID1 = SObjectCfg::objectCfg(1, 1, SObjectId::DTYPE_INT8, false, 1, 1);
static constexpr auto OBJCFG_ID2 = SObjectCfg::objectCfg(1, 1, SObjectId::DTYPE_INT8, false, 3, 2);
static constexpr auto OBJCFG_ID3 = SObjectCfg::objectCfg(1, 1, SObjectId::DTYPE_INT8, true, 1, 3);
static constexpr auto OBJCFG_ID4 = SObjectCfg::objectCfg(1, 1, SObjectId::DTYPE_INT8, true, 3, 4);

//***************************************************************************
// Description: mock for CObject
//***************************************************************************

class CObjectMock : public CObject
{
public:
  explicit CObjectMock(CDescObject &desc)
    : CObject(desc) {};
};

//***************************************************************************
// Description: initialzie common Dawn object
//***************************************************************************

static void test_common_object_init()
{
  uint32_t bin[] = {OBJECT_ID, 0};

  CDescObject desc(bin);
  CObjectMock obj1(desc);

  // Object ID

  TEST_ASSERT_EQUAL(obj1.getId().v, OBJECT_ID);
  TEST_ASSERT_EQUAL(obj1.getIdV(), OBJECT_ID);
  TEST_ASSERT_EQUAL(obj1.getType(), 3);
  TEST_ASSERT_EQUAL(obj1.getCls(), 3);
  TEST_ASSERT_EQUAL(obj1.getDtype(), SObjectId::DTYPE_INT32);
  TEST_ASSERT_EQUAL(obj1.getPriv(), 0xee);
  TEST_ASSERT_EQUAL(obj1.getDtypeSize(), 4);
}

// Build the standard cfg-test descriptor binary into the supplied buffer.
// Layout: OBJECT_ID, 4, OBJCFG_ID1+1B, OBJCFG_ID2+3B, OBJCFG_ID3+1B,
// OBJCFG_ID4+3B (all initialised to 0xff).

static void make_cfg_bin(uint32_t bin[14])
{
  bin[0] = OBJECT_ID;
  bin[1] = 4;
  bin[2] = OBJCFG_ID1;
  bin[3] = 0xff;
  bin[4] = OBJCFG_ID2;
  bin[5] = 0xff;
  bin[6] = 0xff;
  bin[7] = 0xff;
  bin[8] = OBJCFG_ID3;
  bin[9] = 0xff;
  bin[10] = OBJCFG_ID4;
  bin[11] = 0xff;
  bin[12] = 0xff;
  bin[13] = 0xff;
}

//***************************************************************************
// Description: setObjConfig on a read-only entry returns -EACCES.
//***************************************************************************

static void test_common_object_cfg_set_ro_rejected()
{
  uint32_t bin[14];
  uint32_t data[4] = {0x01, 0x02, 0x03, 0x04};

  make_cfg_bin(bin);
  CDescObject desc(bin);
  CObjectMock obj(desc);

  TEST_ASSERT_EQUAL(-EACCES, obj.setObjConfig(OBJCFG_ID1, data, 1));
  TEST_ASSERT_EQUAL(-EACCES, obj.setObjConfig(OBJCFG_ID2, data, 1));

  // Bin must be unchanged.

  TEST_ASSERT_EQUAL(0xff, bin[3]);
  TEST_ASSERT_EQUAL(0xff, bin[5]);
}

//***************************************************************************
// Description: setObjConfig with a length that doesn't match the
// descriptor entry returns -EINVAL.
//***************************************************************************

static void test_common_object_cfg_set_wrong_size_rejected()
{
  uint32_t bin[14];
  uint32_t data[4] = {0x01, 0x02, 0x03, 0x04};

  make_cfg_bin(bin);
  CDescObject desc(bin);
  CObjectMock obj(desc);

  TEST_ASSERT_EQUAL(-EINVAL, obj.setObjConfig(OBJCFG_ID3, data, 0));
  TEST_ASSERT_EQUAL(-EINVAL, obj.setObjConfig(OBJCFG_ID4, data, 1));

  // Bin must be unchanged.

  TEST_ASSERT_EQUAL(0xff, bin[9]);
  TEST_ASSERT_EQUAL(0xff, bin[11]);
}

//***************************************************************************
// Description: setObjConfig on an RW entry with the right size writes the
// data into the descriptor.
//***************************************************************************

static void test_common_object_cfg_set_rw_writes()
{
  uint32_t bin[14];
  uint32_t data[4] = {0x01, 0x02, 0x03, 0x04};

  make_cfg_bin(bin);
  CDescObject desc(bin);
  CObjectMock obj(desc);

  TEST_ASSERT_EQUAL(OK, obj.setObjConfig(OBJCFG_ID3, data, 1));
  TEST_ASSERT_EQUAL(0x01, bin[9]);

  TEST_ASSERT_EQUAL(OK, obj.setObjConfig(OBJCFG_ID4, data, 3));
  TEST_ASSERT_EQUAL(0x01, bin[11]);
  TEST_ASSERT_EQUAL(0x02, bin[12]);
  TEST_ASSERT_EQUAL(0x03, bin[13]);
}

//***************************************************************************
// Description: getObjConfig returns the data currently stored in the
// descriptor for both RO and RW entries.
//***************************************************************************

static void test_common_object_cfg_get_returns_stored_data()
{
  uint32_t bin[14];
  uint32_t data[4] = {0x01, 0x02, 0x03, 0x04};
  uint32_t out[4] = {0};

  make_cfg_bin(bin);
  CDescObject desc(bin);
  CObjectMock obj(desc);

  TEST_ASSERT_EQUAL(OK, obj.setObjConfig(OBJCFG_ID3, data, 1));
  TEST_ASSERT_EQUAL(OK, obj.setObjConfig(OBJCFG_ID4, data, 3));

  TEST_ASSERT_EQUAL(OK, obj.getObjConfig(OBJCFG_ID1, out, 1));
  TEST_ASSERT_EQUAL(0xff, out[0]);

  TEST_ASSERT_EQUAL(OK, obj.getObjConfig(OBJCFG_ID2, out, 3));
  TEST_ASSERT_EQUAL(0xff, out[0]);
  TEST_ASSERT_EQUAL(0xff, out[1]);
  TEST_ASSERT_EQUAL(0xff, out[2]);

  TEST_ASSERT_EQUAL(OK, obj.getObjConfig(OBJCFG_ID3, out, 1));
  TEST_ASSERT_EQUAL(1, out[0]);

  TEST_ASSERT_EQUAL(OK, obj.getObjConfig(OBJCFG_ID4, out, 3));
  TEST_ASSERT_EQUAL(1, out[0]);
  TEST_ASSERT_EQUAL(2, out[1]);
  TEST_ASSERT_EQUAL(3, out[2]);
}

//***************************************************************************
// Description: validate objects with default descValidDefault
//***************************************************************************

static void test_common_object_validate_default()
{
  static constexpr auto OBJID_DEFAULT = 0; // Reserved for default

  int ret;

  uint32_t invalid_len1[] = {
    OBJID_DEFAULT, // Object ID
                   // But no config length
  };

  uint32_t invalid_len2[] = {
    OBJID_DEFAULT, // Object ID
    2,             // Config length
                   // But no data left
  };

  uint32_t invalid_len3[] = {
    OBJID_DEFAULT, // Object ID
    2,             // Config length
    0,             // Some data
                   // But still not enough data
  };

  uint32_t valid_len1[] = {
    OBJID_DEFAULT, // Object ID
    0,             // Config length
    3,
    1              // More data not related to the first object
  };

  uint32_t valid_len2[] = {
    OBJID_DEFAULT, // Object ID
    2,             // Config length
    0,             // Some data
    0,             // Some data
  };

  uint32_t valid_len3[] = {
    OBJID_DEFAULT, // Object ID
    2,             // Config length
    0,             // Some data
    0,             // Some data
    9,
    8,
    7,
    6,
    5 // More data not related to first object
      // Correct offset should be returned
  };

  ret = CObject::descValidDefault(invalid_len1, 1);
  TEST_ASSERT_EQUAL(CObject::DESCVALID_ERR_LEN_ALIGN, ret);
  ret = CObject::descValidDefault(invalid_len1, 6);
  TEST_ASSERT_EQUAL(CObject::DESCVALID_ERR_LEN_ALIGN, ret);

  ret = CObject::descValidDefault(invalid_len1, sizeof(invalid_len1));
  TEST_ASSERT_EQUAL(CObject::DESCVALID_ERR_NO_OBJSIZE, ret);

  ret = CObject::descValidDefault(invalid_len2, sizeof(invalid_len2));
  TEST_ASSERT_EQUAL(CObject::DESCVALID_ERR_NO_CFG_HEADER, ret);

  ret = CObject::descValidDefault(invalid_len3, sizeof(invalid_len3));
  TEST_ASSERT_EQUAL(CObject::DESCVALID_ERR_CFG_TRUNCATED, ret);

  ret = CObject::descValidDefault(valid_len1, sizeof(valid_len1));
  TEST_ASSERT_EQUAL(ret, 2);
  TEST_ASSERT_EQUAL(valid_len1[ret], 3);
  TEST_ASSERT_EQUAL(valid_len1[ret + 1], 1);

  ret = CObject::descValidDefault(valid_len2, sizeof(valid_len2));
  TEST_ASSERT_EQUAL(4, ret);

  ret = CObject::descValidDefault(valid_len3, sizeof(valid_len3));
  TEST_ASSERT_EQUAL(ret, 4);
  TEST_ASSERT_EQUAL(valid_len3[ret], 9);
  TEST_ASSERT_EQUAL(valid_len3[ret + 1], 8);
  TEST_ASSERT_EQUAL(valid_len3[ret + 2], 7);
}

//***************************************************************************
// Description: validate descriptor with default descValidDefault
//***************************************************************************

static void test_common_object_validate_desc()
{
  static constexpr auto OBJID_DEFAULT = 0; // Reserved for default

  int ret;

  uint32_t invalid_len1[] = {
    OBJID_DEFAULT, // Object ID
    0,             // No conifg
    OBJID_DEFAULT, // Object ID
    0,             // No conifg
    OBJID_DEFAULT, // Object ID
    1,             // Expect more data
                   // But no more data
  };

  uint32_t invalid_len2[] = {
    OBJID_DEFAULT, // Object ID
    0,             // No conifg
    OBJID_DEFAULT, // Object ID
    0,             // No conifg
    OBJID_DEFAULT, // Object ID
    1,             // Expect more data
    0,             // Config data
    OBJID_DEFAULT, // Object ID
                   // No config items
  };

  uint32_t valid_len1[] = {
    OBJID_DEFAULT, // Object ID
    0,             // No conifg
    OBJID_DEFAULT, // Object ID
    0,             // No conifg
    OBJID_DEFAULT, // Object ID
    0,             // No conifg
  };

  uint32_t valid_len2[] = {
    OBJID_DEFAULT,                                                  // Object ID
    0,                                                              // No conifg
    OBJID_DEFAULT,                                                  // Object ID
    0,                                                              // No conifg
    OBJID_DEFAULT,                                                  // Object ID
    1,                                                              // Config items
    0,                                                              // Config data
    OBJID_DEFAULT,                                                  // Object ID
    2,                                                              // Config items
    0,                                                              // Config data len = 0
    0,                                                              // Config data len = 0
  };

  uint32_t valid_len3[] = {OBJID_DEFAULT,                           // Object ID
                           0,                                       // No conifg
                           OBJID_DEFAULT,                           // Object ID
                           0,                                       // No conifg
                           OBJID_DEFAULT,                           // Object ID
                           1,                                       // Config items
                           0,                                       // Config data
                           OBJID_DEFAULT,                           // Object ID
                           2,                                       // Config items
                           SObjectCfg::objectCfg(0, 0, 0, 0, 1, 0), // Config data len = 1
                           0,
                           SObjectCfg::objectCfg(0, 0, 0, 0, 3, 0), // Config data len = 3
                           0,
                           0,
                           0};

  ret = CObject::validateDesc(invalid_len1, 1);
  TEST_ASSERT_EQUAL(CObject::DESCVALID_ERR_LEN_ALIGN, ret);
  ret = CObject::validateDesc(invalid_len1, 5);
  TEST_ASSERT_EQUAL(CObject::DESCVALID_ERR_LEN_ALIGN, ret);

  ret = CObject::validateDesc(invalid_len1, sizeof(invalid_len1));
  TEST_ASSERT_EQUAL(CObject::DESCVALID_ERR_NO_CFG_HEADER, ret);

  ret = CObject::validateDesc(invalid_len2, sizeof(invalid_len2));
  TEST_ASSERT_EQUAL(CObject::DESCVALID_ERR_NO_OBJSIZE, ret);

  ret = CObject::validateDesc(valid_len1, sizeof(valid_len1));
  TEST_ASSERT_EQUAL(OK, ret);

  ret = CObject::validateDesc(valid_len2, sizeof(valid_len2));
  TEST_ASSERT_EQUAL(OK, ret);

  ret = CObject::validateDesc(valid_len3, sizeof(valid_len3));
  TEST_ASSERT_EQUAL(OK, ret);
}

extern "C"
{
  int test_common_object()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_common_object_init);
    DAWN_RUN_TEST(test_common_object_cfg_set_ro_rejected);
    DAWN_RUN_TEST(test_common_object_cfg_set_wrong_size_rejected);
    DAWN_RUN_TEST(test_common_object_cfg_set_rw_writes);
    DAWN_RUN_TEST(test_common_object_cfg_get_returns_stored_data);
    DAWN_RUN_TEST(test_common_object_validate_default);
    DAWN_RUN_TEST(test_common_object_validate_desc);

    return UNITY_END();
  }
}
