// dawn/tests/io/test_config.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/config.hxx"
#include "dawn/io/dummy.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto DUMMY1_IO = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 0);
static constexpr auto DUMMY2_IO = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 2);
static constexpr auto DUMMY3_IO = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 2);
static constexpr auto DUMMY4_IO = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 4);
static constexpr auto DUMMY5_IO = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 5);
static constexpr auto DUMMY6_IO = CIODummy::objectId(SObjectId::DTYPE_INT8, false, 6);
static constexpr auto DUMMY7_IO = CIODummy::objectId(SObjectId::DTYPE_FLOAT, false, 7);
static constexpr auto DUMMY8_IO = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 8);

static constexpr auto DUMMY_CFG_RO = CIODummy::cfgIdInitval(SObjectId::DTYPE_UINT32, false, 1);
static constexpr auto DUMMY_CFG_RW = CIODummy::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 1);
static constexpr auto DUMMY_CFG_I8_RW = CIODummy::cfgIdInitval(SObjectId::DTYPE_INT8, true, 1);
static constexpr auto DUMMY_CFG_F32_RW = CIODummy::cfgIdInitval(SObjectId::DTYPE_FLOAT, true, 1);
static constexpr auto DUMMY_CFG_U32X4_RW = CIODummy::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 4);

//***************************************************************************
// Description: config IO with read RO object
//***************************************************************************

static uint32_t g_cfg_config1[] = {
  CIOConfig::objectId(SObjectId::DTYPE_UINT32, 0),
  2,
  CIOConfig::cfgIdCfg(),
  DUMMY_CFG_RO,
  CIOConfig::cfgIdAlloc(SObjectId::DTYPE_UINT32, false, 1),
  DUMMY1_IO,
};

//***************************************************************************
// Description: config IO with RW object
//***************************************************************************

static uint32_t g_cfg_config2[] = {
  CIOConfig::objectId(SObjectId::DTYPE_UINT32, 2),
  2,
  CIOConfig::cfgIdCfg(),
  DUMMY_CFG_RW,
  CIOConfig::cfgIdAlloc(SObjectId::DTYPE_UINT32, false, 1),
  DUMMY2_IO,
};

//***************************************************************************
// Description: config IO with many RW objects
//***************************************************************************

static uint32_t g_cfg_config3[] = {
  CIOConfig::objectId(SObjectId::DTYPE_UINT32, 3),
  2,
  CIOConfig::cfgIdCfg(),
  DUMMY_CFG_RW,
  CIOConfig::cfgIdAlloc(SObjectId::DTYPE_UINT32, false, 2),
  DUMMY2_IO,
  DUMMY3_IO,
};

//***************************************************************************
// Description: config IO with many targets (rollback test path)
//***************************************************************************

static uint32_t g_cfg_config4[] = {
  CIOConfig::objectId(SObjectId::DTYPE_UINT32, 4),
  2,
  CIOConfig::cfgIdCfg(),
  DUMMY_CFG_RW,
  CIOConfig::cfgIdAlloc(SObjectId::DTYPE_UINT32, false, 2),
  DUMMY4_IO,
  DUMMY5_IO,
};

//***************************************************************************
// Description: config IO with single target and limits
//***************************************************************************

static uint32_t g_cfg_config5[] = {
  CIOConfig::objectId(SObjectId::DTYPE_UINT32, 5),
  5,
  CIOCommon::cfgIdLimitMin(SObjectId::DTYPE_UINT32, 1),
  0,
  CIOCommon::cfgIdLimitMax(SObjectId::DTYPE_UINT32, 1),
  10,
  CIOCommon::cfgIdLimitStep(SObjectId::DTYPE_UINT32, 1),
  1,
  CIOConfig::cfgIdCfg(),
  DUMMY_CFG_RW,
  CIOConfig::cfgIdAlloc(SObjectId::DTYPE_UINT32, false, 1),
  DUMMY4_IO,
};

//***************************************************************************
// Description: config IO with single int8 target and limits
//***************************************************************************

static uint32_t g_cfg_config6[] = {
  CIOConfig::objectId(SObjectId::DTYPE_INT8, 6),
  5,
  CIOCommon::cfgIdLimitMin(SObjectId::DTYPE_INT8, 1),
  SObjectCfg::i32ToCfg(-5),
  CIOCommon::cfgIdLimitMax(SObjectId::DTYPE_INT8, 1),
  SObjectCfg::i32ToCfg(5),
  CIOCommon::cfgIdLimitStep(SObjectId::DTYPE_INT8, 1),
  SObjectCfg::i32ToCfg(2),
  CIOConfig::cfgIdCfg(),
  DUMMY_CFG_I8_RW,
  CIOConfig::cfgIdAlloc(SObjectId::DTYPE_INT8, false, 1),
  DUMMY6_IO,
};

//***************************************************************************
// Description: config IO with single float target and limits
//***************************************************************************

static uint32_t g_cfg_config7[] = {
  CIOConfig::objectId(SObjectId::DTYPE_FLOAT, 7),
  5,
  CIOCommon::cfgIdLimitMin(SObjectId::DTYPE_FLOAT, 1),
  SObjectCfg::fToCfg(0.0f),
  CIOCommon::cfgIdLimitMax(SObjectId::DTYPE_FLOAT, 1),
  SObjectCfg::fToCfg(1.0f),
  CIOCommon::cfgIdLimitStep(SObjectId::DTYPE_FLOAT, 1),
  SObjectCfg::fToCfg(0.25f),
  CIOConfig::cfgIdCfg(),
  DUMMY_CFG_F32_RW,
  CIOConfig::cfgIdAlloc(SObjectId::DTYPE_FLOAT, false, 1),
  DUMMY7_IO,
};

//***************************************************************************
// Description: config IO with multi-dimensional uint32 target and limits
//***************************************************************************

static uint32_t g_cfg_config8[] = {
  CIOConfig::objectId(SObjectId::DTYPE_UINT32, 8),
  6,
  CIOCommon::cfgIdLimitMin(SObjectId::DTYPE_UINT32, 4),
  0,
  10,
  0,
  10,
  CIOCommon::cfgIdLimitMax(SObjectId::DTYPE_UINT32, 4),
  1,
  100,
  1,
  100,
  CIOCommon::cfgIdLimitStep(SObjectId::DTYPE_UINT32, 4),
  1,
  10,
  1,
  10,
  CIOConfig::cfgIdCfg(),
  DUMMY_CFG_U32X4_RW,
  CIOConfig::cfgIdAlloc(SObjectId::DTYPE_UINT32, false, 1),
  DUMMY8_IO,
};

//***************************************************************************
// Description: dummy IOs
//***************************************************************************

static uint32_t g_cfg_dummy1[] = {
  DUMMY1_IO,
  1,
  DUMMY_CFG_RO,
  0xff,
};

static uint32_t g_cfg_dummy2[] = {
  DUMMY2_IO,
  1,
  DUMMY_CFG_RW,
  0xee,
};

static uint32_t g_cfg_dummy3[] = {
  DUMMY3_IO,
  1,
  DUMMY_CFG_RW,
  0xaa,
};

static uint32_t g_cfg_dummy4_rw[] = {
  DUMMY4_IO,
  1,
  DUMMY_CFG_RW,
  0x33,
};

static uint32_t g_cfg_dummy5_ro[] = {
  DUMMY5_IO,
  1,
  DUMMY_CFG_RO,
  0x44,
};

static uint32_t g_cfg_dummy6_i8_rw[] = {
  DUMMY6_IO,
  1,
  DUMMY_CFG_I8_RW,
  SObjectCfg::i32ToCfg(-1),
};

static uint32_t g_cfg_dummy7_f32_rw[] = {
  DUMMY7_IO,
  1,
  DUMMY_CFG_F32_RW,
  SObjectCfg::fToCfg(0.5f),
};

static uint32_t g_cfg_dummy8_u32x4_rw[] = {
  DUMMY8_IO,
  2,
  CIODummy::cfgIdDim(),
  4,
  DUMMY_CFG_U32X4_RW,
  0,
  20,
  1,
  30,
};

//***************************************************************************
// Dedicated dummy for offset tests (isolated from multidim contamination)
//***************************************************************************

static constexpr auto DUMMY9_IO = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 9);

static uint32_t g_cfg_dummy9_u32x4_rw[] = {
  DUMMY9_IO,
  2,
  CIODummy::cfgIdDim(),
  4,
  DUMMY_CFG_U32X4_RW,
  0,
  20,
  1,
  30,
};

//***************************************************************************
// Description: config IO with offset=1 size=1 on a 4-word target
//***************************************************************************

static uint32_t g_cfg_config9[] = {
  CIOConfig::objectId(SObjectId::DTYPE_UINT32, 9),
  4,
  CIOConfig::cfgIdCfg(),
  DUMMY_CFG_U32X4_RW,
  CIOConfig::cfgIdAlloc(SObjectId::DTYPE_UINT32, false, 1),
  DUMMY9_IO,
  CIOConfig::cfgIdOffset(),
  1,
  CIOConfig::cfgIdSize(),
  1,
};

//***************************************************************************
// Description: config IO with offset=5 (out of bounds) on a 4-word target
//***************************************************************************

static uint32_t g_cfg_config10[] = {
  CIOConfig::objectId(SObjectId::DTYPE_UINT32, 10),
  4,
  CIOConfig::cfgIdCfg(),
  DUMMY_CFG_U32X4_RW,
  CIOConfig::cfgIdAlloc(SObjectId::DTYPE_UINT32, false, 1),
  DUMMY9_IO,
  CIOConfig::cfgIdOffset(),
  5,
  CIOConfig::cfgIdSize(),
  1,
};

//***************************************************************************
// Description: config IO with offset=1 size=4 (offset+size exceeds field)
//***************************************************************************

static uint32_t g_cfg_config11[] = {
  CIOConfig::objectId(SObjectId::DTYPE_UINT32, 11),
  4,
  CIOConfig::cfgIdCfg(),
  DUMMY_CFG_U32X4_RW,
  CIOConfig::cfgIdAlloc(SObjectId::DTYPE_UINT32, false, 1),
  DUMMY9_IO,
  CIOConfig::cfgIdOffset(),
  1,
  CIOConfig::cfgIdSize(),
  4,
};

//***************************************************************************
// Description: test config IO with read only access
//***************************************************************************

static void test_io_config_ro()
{
  CDescObject desc(g_cfg_config1);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy1);
  CIODummy dummy1(desc1);
  io_sdata_t<uint32_t, 1> data;
  int ret;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());

  // Get binded

  TEST_ASSERT(cfg.map[DUMMY1_IO] == nullptr);

  // Bind config

  ret = cfg.bind(&dummy1, DUMMY1_IO);
  TEST_ASSERT_EQUAL(OK, ret);

  TEST_ASSERT(cfg.map[DUMMY1_IO] != nullptr);
  TEST_ASSERT(cfg.map[DUMMY1_IO]->getIdV() == DUMMY1_IO);

  // Get config item

  TEST_ASSERT_EQUAL(OK, cfg.getData(data, 1));
  TEST_ASSERT_EQUAL(0xff, data(0));

  // Try to modify config item - should fail

  data(0) = 0x11;
  TEST_ASSERT_EQUAL(-EACCES, cfg.setData(data));
}

//***************************************************************************
// Description: test config IO with read-write access
//***************************************************************************

static void test_io_config_rw()
{
  CDescObject desc(g_cfg_config2);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy2);
  CIODummy dummy1(desc1);
  io_sdata_t<uint32_t, 1> data;
  int ret;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());

  // Get binded

  TEST_ASSERT(cfg.map[DUMMY2_IO] == nullptr);

  // Bind config

  ret = cfg.bind(&dummy1, DUMMY2_IO);
  TEST_ASSERT_EQUAL(ret, OK);

  TEST_ASSERT(cfg.map[DUMMY2_IO] != nullptr);
  TEST_ASSERT(cfg.map[DUMMY2_IO]->getIdV() == DUMMY2_IO);

  // Get config item

  TEST_ASSERT(cfg.getData(data, 1) == OK);
  TEST_ASSERT_EQUAL(0xee, data(0));

  // Try to modify config item - should success

  data(0) = 0x22;
  TEST_ASSERT_EQUAL(OK, cfg.setData(data));

  // Get config item and check if updated

  TEST_ASSERT(cfg.getData(data, 1) == OK);
  TEST_ASSERT_EQUAL(0x22, data(0));
}

//***************************************************************************
// Description: test config IO with read-write access to many IO
//***************************************************************************

static void test_io_config_rw_many()
{
  CDescObject desc(g_cfg_config3);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy2);
  CIODummy dummy1(desc1);
  CDescObject desc2(g_cfg_dummy3);
  CIODummy dummy2(desc2);
  io_sdata_t<uint32_t, 1> data;
  int ret;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  TEST_ASSERT_EQUAL(OK, dummy2.configure());
  TEST_ASSERT_EQUAL(OK, dummy2.init());

  // Get binded

  TEST_ASSERT(cfg.map[DUMMY2_IO] == nullptr);
  TEST_ASSERT(cfg.map[DUMMY3_IO] == nullptr);

  // Bind config

  ret = cfg.bind(&dummy1, DUMMY2_IO);
  TEST_ASSERT_EQUAL(ret, OK);
  ret = cfg.bind(&dummy2, DUMMY3_IO);
  TEST_ASSERT_EQUAL(ret, OK);

  TEST_ASSERT(cfg.map[DUMMY2_IO] != nullptr);
  TEST_ASSERT(cfg.map[DUMMY2_IO]->getIdV() == DUMMY2_IO);
  TEST_ASSERT(cfg.map[DUMMY3_IO] != nullptr);
  TEST_ASSERT(cfg.map[DUMMY3_IO]->getIdV() == DUMMY3_IO);

  // Get config item
  // Contract: when multiple objects are bound, getData() returns the first
  // bound object's view (configuration is assumed coherent across bindings).

  TEST_ASSERT(cfg.getData(data, 1) == OK);
  TEST_ASSERT_EQUAL(0xaa, data(0));

  // Try to modify config item - should success

  data(0) = 0x11;
  TEST_ASSERT_EQUAL(OK, cfg.setData(data));

  // Get config item and check if updated

  TEST_ASSERT_EQUAL(OK, cfg.getData(data, 1));
  TEST_ASSERT_EQUAL(0x11, data(0));
}

//***************************************************************************
// Description: config IO rolls back already-applied writes on later failure
//***************************************************************************

static void test_io_config_rw_many_rollback()
{
  CDescObject desc(g_cfg_config4);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy4_rw);
  CIODummy dummy_rw(desc1);
  CDescObject desc2(g_cfg_dummy5_ro);
  CIODummy dummy_ro(desc2);
  io_sdata_t<uint32_t, 1> data;
  int ret;

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy_rw.configure());
  TEST_ASSERT_EQUAL(OK, dummy_rw.init());
  TEST_ASSERT_EQUAL(OK, dummy_ro.configure());
  TEST_ASSERT_EQUAL(OK, dummy_ro.init());

  TEST_ASSERT_EQUAL(OK, cfg.bind(&dummy_rw, DUMMY4_IO));
  TEST_ASSERT_EQUAL(OK, cfg.bind(&dummy_ro, DUMMY5_IO));

  data(0) = 0x77;
  ret = cfg.setData(data);
  TEST_ASSERT(ret < 0);

  TEST_ASSERT_EQUAL(OK, dummy_rw.getConfig(DUMMY_CFG_RW, &data(0), 1));
  TEST_ASSERT_EQUAL(0x33, data(0));
}

//***************************************************************************
// Description: config IO rejects an out-of-range uint32 with -ERANGE and
// leaves the bound dummy untouched.
//***************************************************************************

static void test_io_config_limit_check_uint32_reject()
{
  CDescObject desc(g_cfg_config5);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy4_rw);
  CIODummy dummy_limit(desc1);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy_limit.configure());
  TEST_ASSERT_EQUAL(OK, dummy_limit.init());
  TEST_ASSERT_EQUAL(OK, cfg.bind(&dummy_limit, DUMMY4_IO));

  data(0) = 11;
  TEST_ASSERT_EQUAL(-ERANGE, cfg.setData(data));

  TEST_ASSERT_EQUAL(OK, dummy_limit.getConfig(DUMMY_CFG_RW, &data(0), 1));
  TEST_ASSERT_EQUAL(0x33, data(0));
}

//***************************************************************************
// Description: config IO rejects an out-of-range int8 with -ERANGE.
//***************************************************************************

static void test_io_config_limit_check_int8_reject()
{
  CDescObject desc(g_cfg_config6);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy6_i8_rw);
  CIODummy dummy_limit(desc1);
  io_sdata_t<int8_t, 1> data;

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy_limit.configure());
  TEST_ASSERT_EQUAL(OK, dummy_limit.init());
  TEST_ASSERT_EQUAL(OK, cfg.bind(&dummy_limit, DUMMY6_IO));

  data(0) = 4;
  TEST_ASSERT_EQUAL(-ERANGE, cfg.setData(data));
}

//***************************************************************************
// Description: config IO accepts an in-range int8 and the value reaches the
// underlying dummy.
//***************************************************************************

static void test_io_config_limit_check_int8_accept()
{
  CDescObject desc(g_cfg_config6);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy6_i8_rw);
  CIODummy dummy_limit(desc1);
  io_sdata_t<int8_t, 1> data;
  uint32_t raw = 0;

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy_limit.configure());
  TEST_ASSERT_EQUAL(OK, dummy_limit.init());
  TEST_ASSERT_EQUAL(OK, cfg.bind(&dummy_limit, DUMMY6_IO));

  data(0) = -3;
  TEST_ASSERT_EQUAL(OK, cfg.setData(data));

  TEST_ASSERT_EQUAL(OK, dummy_limit.getConfig(DUMMY_CFG_I8_RW, &raw, 1));
  TEST_ASSERT_EQUAL(SObjectCfg::i32ToCfg(-3), raw);
}

//***************************************************************************
// Description: config IO rejects an out-of-range float with -ERANGE.
//***************************************************************************

static void test_io_config_limit_check_float_reject()
{
  CDescObject desc(g_cfg_config7);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy7_f32_rw);
  CIODummy dummy_limit(desc1);
  io_sdata_t<float, 1> data;

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy_limit.configure());
  TEST_ASSERT_EQUAL(OK, dummy_limit.init());
  TEST_ASSERT_EQUAL(OK, cfg.bind(&dummy_limit, DUMMY7_IO));

  data(0) = 0.60f;
  TEST_ASSERT_EQUAL(-ERANGE, cfg.setData(data));
}

//***************************************************************************
// Description: config IO accepts an in-range float and the value reaches the
// underlying dummy (within float tolerance).
//***************************************************************************

static void test_io_config_limit_check_float_accept()
{
  CDescObject desc(g_cfg_config7);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy7_f32_rw);
  CIODummy dummy_limit(desc1);
  io_sdata_t<float, 1> data;
  uint32_t raw = 0;
  float v;

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy_limit.configure());
  TEST_ASSERT_EQUAL(OK, dummy_limit.init());
  TEST_ASSERT_EQUAL(OK, cfg.bind(&dummy_limit, DUMMY7_IO));

  data(0) = 0.75f;
  TEST_ASSERT_EQUAL(OK, cfg.setData(data));

  TEST_ASSERT_EQUAL(OK, dummy_limit.getConfig(DUMMY_CFG_F32_RW, &raw, 1));
  v = SObjectCfg::cfgToF(raw);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.75f, v);
}

//***************************************************************************
// Description: config IO rejects multidim payload when any element is out of
// the per-element range.
//***************************************************************************

static void test_io_config_limit_check_multidim_reject()
{
  CDescObject desc(g_cfg_config8);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy8_u32x4_rw);
  CIODummy dummy(desc1);
  io_sdata_t<uint32_t, 4> data;

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  TEST_ASSERT_EQUAL(OK, cfg.bind(&dummy, DUMMY8_IO));

  data(0) = 1;
  data(1) = 25; // exceeds element 1 max (10)
  data(2) = 0;
  data(3) = 30;
  TEST_ASSERT_EQUAL(-ERANGE, cfg.setData(data));
}

//***************************************************************************
// Description: config IO accepts a multidim payload when every element is
// in range, and all values reach the underlying dummy.
//***************************************************************************

static void test_io_config_limit_check_multidim_accept()
{
  CDescObject desc(g_cfg_config8);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy8_u32x4_rw);
  CIODummy dummy(desc1);
  io_sdata_t<uint32_t, 4> data;
  uint32_t raw[4];

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  TEST_ASSERT_EQUAL(OK, cfg.bind(&dummy, DUMMY8_IO));

  data(0) = 1;
  data(1) = 20;
  data(2) = 0;
  data(3) = 30;
  TEST_ASSERT_EQUAL(OK, cfg.setData(data));

  TEST_ASSERT_EQUAL(OK, dummy.getConfig(DUMMY_CFG_U32X4_RW, raw, 4));
  TEST_ASSERT_EQUAL(1, raw[0]);
  TEST_ASSERT_EQUAL(20, raw[1]);
  TEST_ASSERT_EQUAL(0, raw[2]);
  TEST_ASSERT_EQUAL(30, raw[3]);
}

//***************************************************************************
// Description: sub-range read with offset=1 size=1 returns only word 1
//***************************************************************************

static void test_io_config_offset_read()
{
  CDescObject desc(g_cfg_config9);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy9_u32x4_rw);
  CIODummy dummy(desc1);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  TEST_ASSERT_EQUAL(OK, cfg.bind(&dummy, DUMMY9_IO));

  TEST_ASSERT_EQUAL(OK, cfg.getData(data, 1));
  TEST_ASSERT_EQUAL(20, data(0));
}

//***************************************************************************
// Description: sub-range write patches only the target word
//***************************************************************************

static void test_io_config_offset_write()
{
  CDescObject desc(g_cfg_config9);
  CIOConfig cfg(desc);
  CDescObject desc1(g_cfg_dummy9_u32x4_rw);
  CIODummy dummy(desc1);
  io_sdata_t<uint32_t, 1> data;
  uint32_t raw[4];

  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.init());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  TEST_ASSERT_EQUAL(OK, cfg.bind(&dummy, DUMMY9_IO));

  // Write 99 to offset 1 (should replace word 1, leave others intact)
  data(0) = 99;
  TEST_ASSERT_EQUAL(OK, cfg.setData(data));

  TEST_ASSERT_EQUAL(OK, dummy.getConfig(DUMMY_CFG_U32X4_RW, raw, 4));
  TEST_ASSERT_EQUAL(0, raw[0]);
  TEST_ASSERT_EQUAL(99, raw[1]);
  TEST_ASSERT_EQUAL(1, raw[2]);
  TEST_ASSERT_EQUAL(30, raw[3]);
}

//***************************************************************************
// Description: out-of-bounds offset is rejected at configure time
//***************************************************************************

static void test_io_config_offset_oob()
{
  CDescObject desc(g_cfg_config10);
  CIOConfig cfg(desc);

  TEST_ASSERT(cfg.configure() < 0);
}

//***************************************************************************
// Description: offset+size exceeding field is rejected at configure time
//***************************************************************************

static void test_io_config_offset_size_oob()
{
  CDescObject desc(g_cfg_config11);
  CIOConfig cfg(desc);

  TEST_ASSERT(cfg.configure() < 0);
}

extern "C"
{
  int test_io_config()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_config_ro);
    DAWN_RUN_TEST(test_io_config_rw);
    DAWN_RUN_TEST(test_io_config_rw_many);
    DAWN_RUN_TEST(test_io_config_rw_many_rollback);

    DAWN_RUN_TEST(test_io_config_limit_check_uint32_reject);

    DAWN_RUN_TEST(test_io_config_limit_check_int8_reject);
    DAWN_RUN_TEST(test_io_config_limit_check_int8_accept);

    DAWN_RUN_TEST(test_io_config_limit_check_float_reject);
    DAWN_RUN_TEST(test_io_config_limit_check_float_accept);

    DAWN_RUN_TEST(test_io_config_limit_check_multidim_reject);
    DAWN_RUN_TEST(test_io_config_limit_check_multidim_accept);

    DAWN_RUN_TEST(test_io_config_offset_read);
    DAWN_RUN_TEST(test_io_config_offset_write);
    DAWN_RUN_TEST(test_io_config_offset_oob);
    DAWN_RUN_TEST(test_io_config_offset_size_oob);

    return UNITY_END();
  }
}
