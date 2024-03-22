// dawn/tests/io/test_virt.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <poll.h>

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "test_common.hxx"

using namespace dawn;

static uint32_t g_cfg_virt[] = {
  CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 0),
  0,
};

static uint32_t g_cfg_virt_ts[] = {
  CIOVirt::objectId(SObjectId::DTYPE_UINT32, true, 0),
  0,
};

// Notifier callback counters

static int g_callback1_cntr;
static int g_callback2_cntr;
static int g_callback3_cntr;
static int g_callback4_cntr;
static int g_callback5_cntr;

// Set/get callback counters

static int g_set_cntr;
static int g_get_cntr;

static int virt_notifier_callback1(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  DAWNASSERT(*(static_cast<uint32_t *>(data->getDataPtr())) == 0xdeadbeef, "invalid data");
  g_callback1_cntr++;
  return OK;
}

static int virt_notifier_callback2(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  DAWNASSERT(*(static_cast<uint32_t *>(data->getDataPtr())) == 0xdeadbeef, "invalid data");
  g_callback2_cntr++;
  return OK;
}

static int virt_notifier_callback3(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  DAWNASSERT(*(static_cast<uint32_t *>(data->getDataPtr())) == 0xdeadbeef, "invalid data");
  g_callback3_cntr++;
  return OK;
}

static int virt_notifier_callback4(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  DAWNASSERT(*(static_cast<uint32_t *>(data->getDataPtr())) == 0xdeadbeef, "invalid data");
  g_callback4_cntr++;
  return OK;
}

static int virt_notifier_callback5(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  DAWNASSERT(*(static_cast<uint32_t *>(data->getDataPtr())) == 0xdeadbeef, "invalid data");
  g_callback5_cntr++;
  return OK;
}

static void virt_set_cb(CIOVirt *io, void *priv)
{
  DAWNASSERT(io != nullptr, "nullptr pointer");
  DAWNASSERT(priv != nullptr, "nullptr pointer");
  UNUSED(io);
  UNUSED(priv);
  g_set_cntr++;
}

static void virt_get_cb(CIOVirt *io, void *priv)
{
  DAWNASSERT(io != nullptr, "nullptr pointer");
  DAWNASSERT(priv != nullptr, "nullptr pointer");
  UNUSED(io);
  UNUSED(priv);
  g_get_cntr++;
}

//***************************************************************************
// Description: getData/setVal on a virtio that hasn't been initialize()d
// returns -EACCES.
//***************************************************************************

static void test_io_virt_uninitialized_rejects()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1> data;
  uint32_t setval = 0;

  TEST_ASSERT_EQUAL(OK, virt.configure());

  // Reject before init()
  TEST_ASSERT_EQUAL(-EACCES, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(-EACCES, virt.setVal(static_cast<void *>(&setval), sizeof(setval)));

  // init() alone does not allocate backing storage
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(-EACCES, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(-EACCES, virt.setVal(static_cast<void *>(&setval), sizeof(setval)));
}

//***************************************************************************
// Description: after initialize() + setVal(), getData() returns the seeded
// value.
//***************************************************************************

static void test_io_virt_init_setval_returns_value()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1> data;
  uint32_t setval = 10;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 1));

  virt.setVal(static_cast<void *>(&setval), sizeof(setval));

  TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(10, data(0));
  TEST_ASSERT_EQUAL(0, data[0]);
}

//***************************************************************************
// Description: initialize() may replace the default backing store created by
// init() and update the virtIO dimension.
//***************************************************************************

static void test_io_virt_reinitialize_updates_dimension()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 4> data;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(0, virt.getDataDim());

  TEST_ASSERT_EQUAL(OK, virt.initialize(4, 1));
  TEST_ASSERT_EQUAL(4, virt.getDataDim());

  data(0) = 10;
  data(1) = 20;
  data(2) = 30;
  data(3) = 40;
  TEST_ASSERT_EQUAL(OK, virt.setData(data));

  data(0) = 0;
  data(1) = 0;
  data(2) = 0;
  data(3) = 0;
  TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(10, data(0));
  TEST_ASSERT_EQUAL(20, data(1));
  TEST_ASSERT_EQUAL(30, data(2));
  TEST_ASSERT_EQUAL(40, data(3));
}

//***************************************************************************
// Description: setData() updates the underlying value and the next read
// returns the new value.
//***************************************************************************

static void test_io_virt_setdata_updates_value()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 1));

  data(0) = 20;
  TEST_ASSERT_EQUAL(OK, virt.setData(data));

  TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(20, data(0));
}

//***************************************************************************
// Description: a batched getData call fills every batch slot with the
// current value (timestamps stay zero in the no-ts variant).
//***************************************************************************

static void test_io_virt_batched_getdata()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1, 10> bdata;
  size_t i;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 1));

  io_sdata_t<uint32_t, 1> seed;
  seed(0) = 20;
  TEST_ASSERT_EQUAL(OK, virt.setData(seed));

  TEST_ASSERT_EQUAL(OK, virt.getData(bdata, 5));
  for (i = 0; i < 5; i++)
    {
      TEST_ASSERT_EQUAL(20, bdata(0, i));
      TEST_ASSERT_EQUAL(0, bdata[i]);
    }
  TEST_ASSERT_EQUAL(0, bdata(0, 5));
  TEST_ASSERT_EQUAL(0, bdata[5]);
}

//***************************************************************************
// Description: with timestamp enabled, getData() returns a non-zero
// timestamp alongside the value.
//***************************************************************************

static void test_io_virt_ts_setdata_updates_value()
{
  CDescObject desc(g_cfg_virt_ts);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1, 1, true> data;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1));

  data(0) = 20;
  TEST_ASSERT_EQUAL(OK, virt.setData(data));

  TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(20, data(0));
  TEST_ASSERT(data[0] > 0);
}

//***************************************************************************
// Description: a batched getData fills value + timestamp for every slot
// when timestamps are enabled.
//***************************************************************************

static void test_io_virt_ts_batched_getdata()
{
  CDescObject desc(g_cfg_virt_ts);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1, 10, true> bdata;
  io_sdata_t<uint32_t, 1, 1, true> seed;
  size_t i;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1));

  seed(0) = 20;
  TEST_ASSERT_EQUAL(OK, virt.setData(seed));

  TEST_ASSERT_EQUAL(OK, virt.getData(bdata, 5));
  for (i = 0; i < 5; i++)
    {
      TEST_ASSERT_EQUAL(20, bdata(0, i));
      TEST_ASSERT(bdata[i] > 0);
    }
  TEST_ASSERT_EQUAL(0, bdata(0, 5));
  TEST_ASSERT_EQUAL(0, bdata[5]);
}

//***************************************************************************
// Description: setData on a virtio with one notifier registered fires the
// notifier callback exactly once.
//***************************************************************************

static void test_io_virt_notify_single()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1> data;

  g_callback1_cntr = 0;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, virt.setNotifier(virt_notifier_callback1, 0, &virt));

  TEST_ASSERT_EQUAL(0, g_callback1_cntr);

  data(0) = 0xdeadbeef;
  TEST_ASSERT_EQUAL(OK, virt.setData(data));
  TEST_ASSERT_EQUAL(1, g_callback1_cntr);

  TEST_ASSERT_EQUAL(OK, virt.setData(data));
  TEST_ASSERT_EQUAL(2, g_callback1_cntr);
}

//***************************************************************************
// Description: setNotifier may be called multiple times; setData fires
// every registered callback for that IO once per call.
//***************************************************************************

static void test_io_virt_notify_multiple_callbacks()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1> data;

  g_callback3_cntr = 0;
  g_callback4_cntr = 0;
  g_callback5_cntr = 0;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, virt.setNotifier(virt_notifier_callback3, 0, &virt));
  TEST_ASSERT_EQUAL(OK, virt.setNotifier(virt_notifier_callback4, 0, &virt));
  TEST_ASSERT_EQUAL(OK, virt.setNotifier(virt_notifier_callback5, 0, &virt));

  data(0) = 0xdeadbeef;
  TEST_ASSERT_EQUAL(OK, virt.setData(data));

  TEST_ASSERT_EQUAL(1, g_callback3_cntr);
  TEST_ASSERT_EQUAL(1, g_callback4_cntr);
  TEST_ASSERT_EQUAL(1, g_callback5_cntr);
}

//***************************************************************************
// Description: setData on one virtio fires that virtio's callback only,
// leaving other virtios' callbacks untouched.
//***************************************************************************

static void test_io_virt_notify_isolation()
{
  CDescObject desc1(g_cfg_virt);
  CDescObject desc2(g_cfg_virt);
  CIOVirt virt1(desc1);
  CIOVirt virt2(desc2);
  io_sdata_t<uint32_t, 1> data;

  g_callback1_cntr = 0;
  g_callback2_cntr = 0;

  TEST_ASSERT_EQUAL(OK, virt1.configure());
  TEST_ASSERT_EQUAL(OK, virt1.init());
  TEST_ASSERT_EQUAL(OK, virt1.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, virt1.setNotifier(virt_notifier_callback1, 0, &virt1));

  TEST_ASSERT_EQUAL(OK, virt2.configure());
  TEST_ASSERT_EQUAL(OK, virt2.init());
  TEST_ASSERT_EQUAL(OK, virt2.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, virt2.setNotifier(virt_notifier_callback2, 0, &virt2));

  data(0) = 0xdeadbeef;
  TEST_ASSERT_EQUAL(OK, virt1.setData(data));
  TEST_ASSERT_EQUAL(1, g_callback1_cntr);
  TEST_ASSERT_EQUAL(0, g_callback2_cntr);

  TEST_ASSERT_EQUAL(OK, virt2.setData(data));
  TEST_ASSERT_EQUAL(1, g_callback1_cntr);
  TEST_ASSERT_EQUAL(1, g_callback2_cntr);
}

//***************************************************************************
// Description: unregistering a virtIO notifier prevents later callbacks.
//***************************************************************************

static void test_io_virt_notify_unregister()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1> data;

  g_callback1_cntr = 0;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, virt.regNotifier({&virt, &virt, virt_notifier_callback1, 0}));

  data(0) = 0xdeadbeef;
  TEST_ASSERT_EQUAL(OK, virt.setData(data));
  TEST_ASSERT_EQUAL(1, g_callback1_cntr);

  TEST_ASSERT_EQUAL(OK, virt.unregNotifier({&virt, &virt, virt_notifier_callback1, 0}));
  TEST_ASSERT_EQUAL(-ENOENT, virt.unregNotifier({&virt, &virt, virt_notifier_callback1, 0}));

  TEST_ASSERT_EQUAL(OK, virt.setData(data));
  TEST_ASSERT_EQUAL(1, g_callback1_cntr);
}

//***************************************************************************
// Description: setNotifier(nullptr) unregisters VirtIO callbacks and later
// notifications do not call through a null callback.
//***************************************************************************

static void test_io_virt_notify_set_notifier_null_unregisters()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1> data;

  g_callback1_cntr = 0;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, virt.setNotifier(virt_notifier_callback1, 0, &virt));

  data(0) = 0xdeadbeef;
  TEST_ASSERT_EQUAL(OK, virt.setData(data));
  TEST_ASSERT_EQUAL(1, g_callback1_cntr);

  TEST_ASSERT_EQUAL(OK, virt.setNotifier(nullptr, 0, nullptr));
  TEST_ASSERT_EQUAL(OK, virt.setData(data));
  TEST_ASSERT_EQUAL(1, g_callback1_cntr);
}

//***************************************************************************
// Description: a setCallbackSet hook fires once per setData call.
//***************************************************************************

static void test_io_virt_set_callback()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1> data;
  int i;

  g_set_cntr = 0;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 1));
  TEST_ASSERT_EQUAL(OK, virt.setCallbackSet(virt_set_cb, (void *)0xdeadbeef));

  data(0) = 10;
  for (i = 0; i < 4; i++)
    {
      TEST_ASSERT_EQUAL(OK, virt.setData(data));
    }

  TEST_ASSERT_EQUAL(4, g_set_cntr);
}

//***************************************************************************
// Description: a setCallbackGet hook fires once per getData call.
//***************************************************************************

static void test_io_virt_get_callback()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1> data;
  int i;

  g_get_cntr = 0;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 1));
  TEST_ASSERT_EQUAL(OK, virt.setCallbackGet(virt_get_cb, (void *)0xdeadbeef));

  for (i = 0; i < 3; i++)
    {
      TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
    }

  TEST_ASSERT_EQUAL(3, g_get_cntr);
}

extern "C"
{
  int test_io_virt()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_virt_uninitialized_rejects);
    DAWN_RUN_TEST(test_io_virt_init_setval_returns_value);
    DAWN_RUN_TEST(test_io_virt_reinitialize_updates_dimension);
    DAWN_RUN_TEST(test_io_virt_setdata_updates_value);
    DAWN_RUN_TEST(test_io_virt_batched_getdata);

    DAWN_RUN_TEST(test_io_virt_ts_setdata_updates_value);
    DAWN_RUN_TEST(test_io_virt_ts_batched_getdata);

    DAWN_RUN_TEST(test_io_virt_notify_single);
    DAWN_RUN_TEST(test_io_virt_notify_multiple_callbacks);
    DAWN_RUN_TEST(test_io_virt_notify_isolation);
    DAWN_RUN_TEST(test_io_virt_notify_unregister);
    DAWN_RUN_TEST(test_io_virt_notify_set_notifier_null_unregisters);

    DAWN_RUN_TEST(test_io_virt_set_callback);
    DAWN_RUN_TEST(test_io_virt_get_callback);

    return UNITY_END();
  }
}
