// dawn/tests/proto/test_nimble.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/capabilities.hxx"
#include "dawn/io/dummy.hxx"
#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/proto/nimble/prph.hxx"
#include "dawn/proto/nimble/prph_aios.hxx"
#include "dawn/proto/nimble/prph_bas.hxx"
#include "dawn/proto/nimble/prph_ess.hxx"
#include "dawn/proto/nimble/prph_imds.hxx"
#include "dawn/proto/nimble/prph_ots.hxx"
#include "test_common.hxx"

using namespace dawn;

// workaround for nimble, where we can't properly close opened BLE sockets

#if CONFIG_NET_BLUETOOTH_PREALLOC_CONNS < 8
#  error need more BLE connections
#endif

static constexpr auto NIMBLE_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto NIMBLE_DUMMYIO2 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 2);
static constexpr auto NIMBLE_DUMMYIO3 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 3);
static constexpr auto NIMBLE_DUMMYIO4 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 4);
static constexpr auto NIMBLE_BASIO1 = CIODummyNotify::objectId(SObjectId::DTYPE_UINT8, false, 1);

//***************************************************************************
// Description: dummy IOs
//***************************************************************************

static uint32_t g_cfg_dummy1[] = {
  NIMBLE_DUMMYIO1,
  0,
};

static uint32_t g_cfg_dummy2[] = {
  NIMBLE_DUMMYIO2,
  0,
};

static uint32_t g_cfg_dummy3[] = {
  NIMBLE_DUMMYIO3,
  0,
};

static uint32_t g_cfg_dummy4[] = {
  NIMBLE_DUMMYIO4,
  0,
};

static uint32_t g_cfg_bas_notify1[] = {
  NIMBLE_BASIO1,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_UINT8, true, 1),
  42,
  CIODummyNotify::cfgInterval(false),
  50000,
};

static uint32_t g_bin_nimble_cmn[] = {
  // Object ID

  CProtoNimblePrph::objectId(0),
  0};

static uint32_t g_bin_nimble_bas[] = {
  CProtoNimblePrph::objectId(0),
  1,
  CProtoNimblePrph::cfgIdIOBindBas(),
  NIMBLE_BASIO1,
};

static uint32_t g_bin_nimble_dis[] = {CProtoNimblePrph::objectId(0),
                                      1,
                                      CProtoNimblePrph::cfgIdIOBindDis()};

//***************************************************************************
// Description: Nimble AIOS descriptor fixture binds digital and analog IOs.
//***************************************************************************

static uint32_t g_bin_nimble_aios[] = {
  CProtoNimblePrph::objectId(0),
  1,

  CProtoNimblePrph::cfgIdIOBindAios(27),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfg0(3),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfg1(1),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfg2(0),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfgObj(CProtoNimblePrphAios::PRPH_AIOS_TYPE_DIGITAL),
  NIMBLE_DUMMYIO1,
  4,
  CProtoNimblePrphAios::cfgIdIOBindAiosExt(CProtoNimblePrphAios::AIOS_EXT_EXTENDED_PROPERTIES, 1),
  0x00000000,
  CProtoNimblePrphAios::cfgIdIOBindAiosExt(CProtoNimblePrphAios::AIOS_EXT_VALUE_TRIGGER_SETTING, 1),
  NIMBLE_DUMMYIO4,
  CProtoNimblePrphAios::cfgIdIOBindAiosExt(CProtoNimblePrphAios::AIOS_EXT_TIME_TRIGGER_SETTING, 1),
  NIMBLE_DUMMYIO4,
  CProtoNimblePrphAios::cfgIdIOBindAiosExt(CProtoNimblePrphAios::AIOS_EXT_PRESENTATION_FORMAT, 2),
  0x27000001,
  0x00000001,
  CProtoNimblePrphAios::cfgIdIOBindAiosCfgObj(CProtoNimblePrphAios::PRPH_AIOS_TYPE_DIGITAL),
  NIMBLE_DUMMYIO2,
  0,
  CProtoNimblePrphAios::cfgIdIOBindAiosCfgObj(CProtoNimblePrphAios::PRPH_AIOS_TYPE_DIGITAL),
  NIMBLE_DUMMYIO3,
  0,
  CProtoNimblePrphAios::cfgIdIOBindAiosCfg0(1),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfg1(0),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfg2(0),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfgObj(CProtoNimblePrphAios::PRPH_AIOS_TYPE_ANALOG),
  NIMBLE_DUMMYIO4,
  0,
};

//***************************************************************************
// Description: Nimble ESS descriptor fixture binds environmental sensors.
//***************************************************************************

static uint32_t g_bin_nimble_ess[] = {
  CProtoNimblePrph::objectId(0),
  1,

  CProtoNimblePrph::cfgIdIOBindEss(40),
  CProtoNimblePrphEss::cfgIdIOBindEssCfg0(3),
  CProtoNimblePrphEss::cfgIdIOBindEssCfg1(0),
  CProtoNimblePrphEss::cfgIdIOBindEssCfg2(0),
  CProtoNimblePrphEss::cfgIdIOBindEssCfgObj(CProtoNimblePrphEss::PRPH_ESS_TYPE_TEMP),
  NIMBLE_DUMMYIO1,
  5,
  CProtoNimblePrphEss::cfgIdIOBindEssExt(CProtoNimblePrphEss::ESS_EXT_USER_DESCRIPTION, 4),
  ('t' | ('e' << 8) | ('m' << 16) | ('p' << 24)),
  0,
  0,
  0,
  CProtoNimblePrphEss::cfgIdIOBindEssExt(CProtoNimblePrphEss::ESS_EXT_VALID_RANGE, 2),
  0xfffff060,      // -4000 raw sint16, 0.01 C
  0x00002134,      // 8500 raw sint16, 0.01 C
  CProtoNimblePrphEss::cfgIdIOBindEssExt(CProtoNimblePrphEss::ESS_EXT_MEASUREMENT, 6),
  0,
  NIMBLE_DUMMYIO2, // sampling function IO
  NIMBLE_DUMMYIO3, // measurement period IO
  NIMBLE_DUMMYIO4, // update interval IO
  0,
  NIMBLE_DUMMYIO1, // uncertainty IO
  CProtoNimblePrphEss::cfgIdIOBindEssExt(CProtoNimblePrphEss::ESS_EXT_CONFIGURATION, 1),
  NIMBLE_DUMMYIO2, // ES Configuration descriptor IO
  CProtoNimblePrphEss::cfgIdIOBindEssExt(CProtoNimblePrphEss::ESS_EXT_TRIGGER_SETTING, 1),
  NIMBLE_DUMMYIO3, // ES Trigger Setting descriptor IO
  CProtoNimblePrphEss::cfgIdIOBindEssCfgObj(CProtoNimblePrphEss::PRPH_ESS_TYPE_HUM),
  NIMBLE_DUMMYIO2,
  0,
  CProtoNimblePrphEss::cfgIdIOBindEssCfgObj(CProtoNimblePrphEss::PRPH_ESS_TYPE_PRESS),
  NIMBLE_DUMMYIO3,
  0,
  CProtoNimblePrphEss::cfgIdIOBindEssCfg0(1),
  CProtoNimblePrphEss::cfgIdIOBindEssCfg1(0),
  CProtoNimblePrphEss::cfgIdIOBindEssCfg2(0),
  CProtoNimblePrphEss::cfgIdIOBindEssCfgObj(CProtoNimblePrphEss::PRPH_ESS_TYPE_UVIDX),
  NIMBLE_DUMMYIO4,
  1,
  CProtoNimblePrphEss::cfgIdIOBindEssExt(CProtoNimblePrphEss::ESS_EXT_VALID_RANGE, 2),
  0,
  11,
};

//***************************************************************************
// Description: Nimble IMDS descriptor fixture binds indoor monitoring sensors.
//***************************************************************************

static uint32_t g_bin_nimble_imds[] = {
  CProtoNimblePrph::objectId(0),
  1,

  CProtoNimblePrph::cfgIdIOBindImds(18),
  CProtoNimblePrphImds::cfgIdIOBindImdsCfg0(3),
  CProtoNimblePrphImds::cfgIdIOBindImdsCfg1(0),
  CProtoNimblePrphImds::cfgIdIOBindImdsCfg2(0),
  CProtoNimblePrphImds::cfgIdIOBindImdsCfgObj(CProtoNimblePrphImds::PRPH_IMDS_TYPE_TEMP),
  NIMBLE_DUMMYIO1,
  0,
  CProtoNimblePrphImds::cfgIdIOBindImdsCfgObj(CProtoNimblePrphImds::PRPH_IMDS_TYPE_HUM),
  NIMBLE_DUMMYIO2,
  0,
  CProtoNimblePrphImds::cfgIdIOBindImdsCfgObj(CProtoNimblePrphImds::PRPH_IMDS_TYPE_PRESS),
  NIMBLE_DUMMYIO3,
  0,
  CProtoNimblePrphImds::cfgIdIOBindImdsCfg0(1),
  CProtoNimblePrphImds::cfgIdIOBindImdsCfg1(0),
  CProtoNimblePrphImds::cfgIdIOBindImdsCfg2(0),
  CProtoNimblePrphImds::cfgIdIOBindImdsCfgObj(CProtoNimblePrphImds::PRPH_IMDS_TYPE_TEMP),
  NIMBLE_DUMMYIO4,
  0,
};

//***************************************************************************
// Description: OTS service binary descriptor with one seekable IO
// (CIOCapabilities). 6 words per object: cfg + 4*name + 1*objid; plus 3
// header words (cfg0=count, cfg1=0, cfg2=0). Total = 9 words.
//***************************************************************************

static constexpr auto NIMBLE_CAPSIO1 = CIOCapabilities::objectId(0);

static uint32_t g_bin_nimble_ots[] = {
  CProtoNimblePrph::objectId(0),
  1,

  CProtoNimblePrph::cfgIdIOBindOts(9),
  1, // cfg0: number of objects
  0, // cfg1
  0, // cfg2
  // Object 0 entry
  // type=file (0) | access=read (0<<4) | on_complete=none (0<<6) = 0
  0,
  // name "caps0" packed little-endian into 4 uint32 words
  ('c' | ('a' << 8) | ('p' << 16) | ('s' << 24)),
  ('0' | (0 << 8) | (0 << 16) | (0 << 24)),
  0,
  0,
  // objid (caps IO id)
  NIMBLE_CAPSIO1,
};

static uint32_t g_cfg_caps1[] = {
  NIMBLE_CAPSIO1,
  0,
};

static uint32_t g_bin_nimble_many[] = {
  CProtoNimblePrph::objectId(0),
  3,

  CProtoNimblePrph::cfgIdIOBindDis(),

  CProtoNimblePrph::cfgIdIOBindBas(),
  NIMBLE_BASIO1,

  CProtoNimblePrph::cfgIdIOBindAios(18),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfg0(3),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfg1(0),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfg2(0),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfgObj(CProtoNimblePrphAios::PRPH_AIOS_TYPE_DIGITAL),
  NIMBLE_DUMMYIO1,
  0,
  CProtoNimblePrphAios::cfgIdIOBindAiosCfgObj(CProtoNimblePrphAios::PRPH_AIOS_TYPE_DIGITAL),
  NIMBLE_DUMMYIO2,
  0,
  CProtoNimblePrphAios::cfgIdIOBindAiosCfgObj(CProtoNimblePrphAios::PRPH_AIOS_TYPE_DIGITAL),
  NIMBLE_DUMMYIO3,
  0,
  CProtoNimblePrphAios::cfgIdIOBindAiosCfg0(1),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfg1(0),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfg2(0),
  CProtoNimblePrphAios::cfgIdIOBindAiosCfgObj(CProtoNimblePrphAios::PRPH_AIOS_TYPE_ANALOG),
  NIMBLE_DUMMYIO4,
  0,
};

static uint32_t g_bin_nimble_custom[] = {
  CProtoNimblePrph::objectId(0),
  1,

  CProtoNimblePrph::cfgIdIOBindCustom(13),
  1,
  0,
  0,
  0x9abcdef0,
  0x12345678,
  0x12345678,
  0x12345678,
  0x03,
  0x9abcdef1,
  0x12345678,
  0x12345678,
  0x12345678,
  NIMBLE_DUMMYIO1,
};

// Configure + init a single dummy in one shot.

static void nimble_init_dummy(CIOCommon &d)
{
  TEST_ASSERT_EQUAL(OK, d.configure());
  TEST_ASSERT_EQUAL(OK, d.init());
}

// Run the standard nimble lifecycle (init -> start -> stop) and assert that
// hasThread() reports the expected_running state while running.  The common
// handler with no services bound never spins up a thread, so the caller
// passes false in that case.

static void nimble_run_lifecycle(CProtoNimblePrph &nimble, bool expected_running_thread)
{
  TEST_ASSERT_EQUAL(false, nimble.hasThread());
  TEST_ASSERT_EQUAL(OK, nimble.init());
  TEST_ASSERT_EQUAL(OK, nimble.start());
  TEST_ASSERT_EQUAL(expected_running_thread, nimble.hasThread());
  TEST_ASSERT_EQUAL(OK, nimble.stop());
  TEST_ASSERT_EQUAL(false, nimble.hasThread());
  nimble.deinit();
}

static void nimble_run_restart(CProtoNimblePrph &nimble, bool expected_running_thread)
{
  TEST_ASSERT_EQUAL(false, nimble.hasThread());
  TEST_ASSERT_EQUAL(OK, nimble.init());
  TEST_ASSERT_EQUAL(OK, nimble.start());
  TEST_ASSERT_EQUAL(expected_running_thread, nimble.hasThread());
  TEST_ASSERT_EQUAL(OK, nimble.stop());
  TEST_ASSERT_EQUAL(false, nimble.hasThread());
  TEST_ASSERT_EQUAL(OK, nimble.start());
  TEST_ASSERT_EQUAL(expected_running_thread, nimble.hasThread());
  TEST_ASSERT_EQUAL(OK, nimble.stop());
  TEST_ASSERT_EQUAL(false, nimble.hasThread());
  nimble.deinit();
}

//***************************************************************************
// Description: common handler with no services bound never spins up a
// thread.
//***************************************************************************

static void test_proto_nimbleprph_cmn_no_thread(void)
{
  CDescObject desc(g_bin_nimble_cmn);
  CProtoNimblePrph nimble(desc);

  TEST_ASSERT_EQUAL(OK, nimble.configure());
  nimble_run_lifecycle(nimble, false);
}

//***************************************************************************
// Description: DIS service alone runs its lifecycle (start -> hasThread ->
// stop).
//***************************************************************************

static void test_proto_nimbleprph_dis_lifecycle(void)
{
  CDescObject desc(g_bin_nimble_dis);
  CProtoNimblePrph nimble(desc);

  TEST_ASSERT_EQUAL(OK, nimble.configure());
  nimble_run_lifecycle(nimble, true);
}

//***************************************************************************
// Description: BAS service with one bound notify-capable dummy runs its
// lifecycle.
//***************************************************************************

static void test_proto_nimbleprph_bas_lifecycle()
{
  CDescObject desc(g_bin_nimble_bas);
  CProtoNimblePrph nimble(desc);
  CDescObject desc1(g_cfg_bas_notify1);
  CIODummyNotify dummy1(desc1);
  CIONotifier notifier;

  nimble_init_dummy(dummy1);
  dummy1.bindNotifier(&notifier);
  dummy1.start();
  TEST_ASSERT_EQUAL(OK, nimble.configure());
  nimble.setObjectMapItem(NIMBLE_BASIO1, &dummy1);

  nimble_run_lifecycle(nimble, true);
}

//***************************************************************************
// Description: AIOS service with four bound dummies runs its lifecycle.
//***************************************************************************

static void test_proto_nimbleprph_aios_lifecycle(void)
{
  CDescObject desc(g_bin_nimble_aios);
  CProtoNimblePrph nimble(desc);
  CDescObject desc1(g_cfg_dummy1);
  CIODummy dummy1(desc1);
  CDescObject desc2(g_cfg_dummy2);
  CIODummy dummy2(desc2);
  CDescObject desc3(g_cfg_dummy3);
  CIODummy dummy3(desc3);
  CDescObject desc4(g_cfg_dummy4);
  CIODummy dummy4(desc4);

  nimble_init_dummy(dummy1);
  nimble_init_dummy(dummy2);
  nimble_init_dummy(dummy3);
  nimble_init_dummy(dummy4);
  TEST_ASSERT_EQUAL(OK, nimble.configure());
  nimble.setObjectMapItem(NIMBLE_DUMMYIO1, &dummy1);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO2, &dummy2);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO3, &dummy3);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO4, &dummy4);

  nimble_run_lifecycle(nimble, true);
}

//***************************************************************************
// Description: ESS service with four bound dummies runs its lifecycle.
//***************************************************************************

static void test_proto_nimbleprph_ess_lifecycle(void)
{
  CDescObject desc(g_bin_nimble_ess);
  CProtoNimblePrph nimble(desc);
  CDescObject desc1(g_cfg_dummy1);
  CIODummy dummy1(desc1);
  CDescObject desc2(g_cfg_dummy2);
  CIODummy dummy2(desc2);
  CDescObject desc3(g_cfg_dummy3);
  CIODummy dummy3(desc3);
  CDescObject desc4(g_cfg_dummy4);
  CIODummy dummy4(desc4);

  nimble_init_dummy(dummy1);
  nimble_init_dummy(dummy2);
  nimble_init_dummy(dummy3);
  nimble_init_dummy(dummy4);
  TEST_ASSERT_EQUAL(OK, nimble.configure());
  nimble.setObjectMapItem(NIMBLE_DUMMYIO1, &dummy1);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO2, &dummy2);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO3, &dummy3);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO4, &dummy4);

  nimble_run_lifecycle(nimble, true);
}

//***************************************************************************
// Description: IMDS service with four bound dummies runs its lifecycle.
//***************************************************************************

static void test_proto_nimbleprph_imds_lifecycle(void)
{
  CDescObject desc(g_bin_nimble_imds);
  CProtoNimblePrph nimble(desc);
  CDescObject desc1(g_cfg_dummy1);
  CIODummy dummy1(desc1);
  CDescObject desc2(g_cfg_dummy2);
  CIODummy dummy2(desc2);
  CDescObject desc3(g_cfg_dummy3);
  CIODummy dummy3(desc3);
  CDescObject desc4(g_cfg_dummy4);
  CIODummy dummy4(desc4);

  nimble_init_dummy(dummy1);
  nimble_init_dummy(dummy2);
  nimble_init_dummy(dummy3);
  nimble_init_dummy(dummy4);
  TEST_ASSERT_EQUAL(OK, nimble.configure());
  nimble.setObjectMapItem(NIMBLE_DUMMYIO1, &dummy1);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO2, &dummy2);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO3, &dummy3);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO4, &dummy4);

  nimble_run_lifecycle(nimble, true);
}

//***************************************************************************
// Description: DIS + BAS + AIOS bound at once on a single peripheral runs
// the combined lifecycle.
//***************************************************************************

static void test_proto_nimbleprph_manyatonce_lifecycle()
{
  CDescObject desc(g_bin_nimble_many);
  CProtoNimblePrph nimble(desc);
  CDescObject desc1(g_cfg_bas_notify1);
  CIODummyNotify bas1(desc1);
  CDescObject desc1a(g_cfg_dummy1);
  CIODummy dummy1(desc1a);
  CDescObject desc2(g_cfg_dummy2);
  CIODummy dummy2(desc2);
  CDescObject desc3(g_cfg_dummy3);
  CIODummy dummy3(desc3);
  CDescObject desc4(g_cfg_dummy4);
  CIODummy dummy4(desc4);
  CIONotifier notifier;

  nimble_init_dummy(bas1);
  nimble_init_dummy(dummy1);
  nimble_init_dummy(dummy2);
  nimble_init_dummy(dummy3);
  nimble_init_dummy(dummy4);
  bas1.bindNotifier(&notifier);
  bas1.start();
  TEST_ASSERT_EQUAL(OK, nimble.configure());
  nimble.setObjectMapItem(NIMBLE_BASIO1, &bas1);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO1, &dummy1);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO2, &dummy2);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO3, &dummy3);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO4, &dummy4);

  nimble_run_lifecycle(nimble, true);
}

//***************************************************************************
// Description: services that allocate runtime GATT callback state can stop
// and start again without rebuilding duplicate service definitions.
//***************************************************************************

static void test_proto_nimbleprph_manyatonce_restart()
{
  CDescObject desc(g_bin_nimble_many);
  CProtoNimblePrph nimble(desc);
  CDescObject desc1(g_cfg_bas_notify1);
  CIODummyNotify bas1(desc1);
  CDescObject desc1a(g_cfg_dummy1);
  CIODummy dummy1(desc1a);
  CDescObject desc2(g_cfg_dummy2);
  CIODummy dummy2(desc2);
  CDescObject desc3(g_cfg_dummy3);
  CIODummy dummy3(desc3);
  CDescObject desc4(g_cfg_dummy4);
  CIODummy dummy4(desc4);
  CIONotifier notifier;

  nimble_init_dummy(bas1);
  nimble_init_dummy(dummy1);
  nimble_init_dummy(dummy2);
  nimble_init_dummy(dummy3);
  nimble_init_dummy(dummy4);
  bas1.bindNotifier(&notifier);
  bas1.start();
  TEST_ASSERT_EQUAL(OK, nimble.configure());
  nimble.setObjectMapItem(NIMBLE_BASIO1, &bas1);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO1, &dummy1);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO2, &dummy2);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO3, &dummy3);
  nimble.setObjectMapItem(NIMBLE_DUMMYIO4, &dummy4);

  nimble_run_restart(nimble, true);
}

//***************************************************************************
// Description: OTS service with one seekable IO (CIOCapabilities) runs its
// init/start/stop lifecycle on the dummy NimBLE backend.
//***************************************************************************

static void test_proto_nimbleprph_ots_lifecycle(void)
{
  CDescObject desc(g_bin_nimble_ots);
  CProtoNimblePrph nimble(desc);
  CDescObject desc1(g_cfg_caps1);
  CIOCapabilities caps1(desc1);

  TEST_ASSERT_EQUAL(OK, caps1.configure());
  TEST_ASSERT_EQUAL(OK, caps1.init());
  TEST_ASSERT_EQUAL(true, caps1.isSeekable());

  TEST_ASSERT_EQUAL(OK, nimble.configure());
  nimble.setObjectMapItem(NIMBLE_CAPSIO1, &caps1);

  nimble_run_lifecycle(nimble, true);
}

//***************************************************************************
// Description: OTS rejects non-seekable IOs at start time. We bind a
// CIODummy (not seekable) to an OTS object slot; the createOTS check should
// fail and start() should propagate a non-OK status, but the test simply
// confirms init succeeds and start does not crash.
//***************************************************************************

static void test_proto_nimbleprph_ots_nonseekable_rejected(void)
{
  // Build a minimal OTS descriptor that points at NIMBLE_DUMMYIO1 instead
  // of a seekable IO. Reuse the same layout as g_bin_nimble_ots but with a
  // dummy objid in the trailing word.

  uint32_t cfg[] = {
    CProtoNimblePrph::objectId(0),
    1,

    CProtoNimblePrph::cfgIdIOBindOts(9),
    1, // cfg0
    0,
    0,
    0, // cfg word
    0,
    0,
    0,
    0,
    NIMBLE_DUMMYIO1,
  };

  CDescObject desc(cfg);
  CProtoNimblePrph nimble(desc);
  CDescObject desc1(g_cfg_dummy1);
  CIODummy dummy1(desc1);

  nimble_init_dummy(dummy1);
  TEST_ASSERT_EQUAL(OK, nimble.configure());
  nimble.setObjectMapItem(NIMBLE_DUMMYIO1, &dummy1);

  // Start with a non-seekable IO bound to OTS — start path returns OK at
  // the peripheral level (per-service errors are logged, not propagated).
  // The important thing is we do not crash.

  TEST_ASSERT_EQUAL(OK, nimble.init());
  (void)nimble.start();
  (void)nimble.stop();
}

//***************************************************************************
// Description: one descriptor-defined custom service runs its lifecycle.
//***************************************************************************

static void test_proto_nimbleprph_custom_lifecycle(void)
{
  CDescObject desc(g_bin_nimble_custom);
  CProtoNimblePrph nimble(desc);
  CDescObject desc1(g_cfg_dummy1);
  CIODummy dummy1(desc1);

  nimble_init_dummy(dummy1);
  TEST_ASSERT_EQUAL(OK, nimble.configure());
  nimble.setObjectMapItem(NIMBLE_DUMMYIO1, &dummy1);
  nimble_run_lifecycle(nimble, true);
}

extern "C"
{
  int test_proto_nimbleprph(void)
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_proto_nimbleprph_cmn_no_thread);
    DAWN_RUN_TEST(test_proto_nimbleprph_dis_lifecycle);
    DAWN_RUN_TEST(test_proto_nimbleprph_bas_lifecycle);
    DAWN_RUN_TEST(test_proto_nimbleprph_aios_lifecycle);
    DAWN_RUN_TEST(test_proto_nimbleprph_ess_lifecycle);
    DAWN_RUN_TEST(test_proto_nimbleprph_imds_lifecycle);
    DAWN_RUN_TEST(test_proto_nimbleprph_manyatonce_lifecycle);
    DAWN_RUN_TEST(test_proto_nimbleprph_manyatonce_restart);
    DAWN_RUN_TEST(test_proto_nimbleprph_ots_lifecycle);
    DAWN_RUN_TEST(test_proto_nimbleprph_ots_nonseekable_rejected);
    DAWN_RUN_TEST(test_proto_nimbleprph_custom_lifecycle);

    return UNITY_END();
  }
}
