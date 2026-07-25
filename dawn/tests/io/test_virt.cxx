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

// Batch count seen by the last notification

static size_t g_notify_batch;

static int virt_notifier_batch(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  g_notify_batch = data->getBatch();
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
// Description: a timestamped multi-batch virtio keeps each batch value and
// its own timestamp across setData/getData.
//***************************************************************************

static void test_io_virt_ts_multibatch_roundtrip()
{
  CDescObject desc(g_cfg_virt_ts);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1, 4, true> wdata;
  io_sdata_t<uint32_t, 1, 4, true> rdata;
  size_t i;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 4));

  for (i = 0; i < 4; i++)
    {
      wdata(0, i) = 100 + i;
      wdata[i] = 1000 + i;
    }

  TEST_ASSERT_EQUAL(OK, virt.setData(wdata));
  TEST_ASSERT_EQUAL(OK, virt.getData(rdata, 4));

  for (i = 0; i < 4; i++)
    {
      TEST_ASSERT_EQUAL(100 + i, rdata(0, i));
      TEST_ASSERT_EQUAL(1000 + i, rdata[i]);
    }
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
// Description: a multi-batch virtio returns each batch as stored, not the
// first one repeated, and rejects a read longer than it holds.
//***************************************************************************

static void test_io_virt_multibatch_getdata()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1, 4> wdata;
  io_sdata_t<uint32_t, 1, 4> rdata;
  io_sdata_t<uint32_t, 1, 8> toolong;
  size_t i;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 4));

  for (i = 0; i < 4; i++)
    {
      wdata(0, i) = 100 + i;
    }

  TEST_ASSERT_EQUAL(OK, virt.setData(wdata));
  TEST_ASSERT_EQUAL(OK, virt.getData(rdata, 4));

  for (i = 0; i < 4; i++)
    {
      TEST_ASSERT_EQUAL(100 + i, rdata(0, i));
    }

  TEST_ASSERT_EQUAL(-EINVAL, virt.getData(toolong, 8));
}

//***************************************************************************
// Description: a multi-batch virtio rejects setData from a buffer holding
// fewer batches than it stores instead of reading past the source.
//***************************************************************************

static void test_io_virt_multibatch_setdata_rejects_short_batch()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1, 1> one;
  io_sdata_t<uint32_t, 1, 4> four;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 4));

  one(0) = 7;
  TEST_ASSERT_EQUAL(-EINVAL, virt.setData(one));

  TEST_ASSERT_EQUAL(OK, virt.setData(four));
}

//***************************************************************************
// Description: a source holding more batches than the virtio stores (a
// batched notifier buffer into a batch-1 target) stores the first ones.
//***************************************************************************

static void test_io_virt_setdata_longer_source_stores_first()
{
  CDescObject desc1(g_cfg_virt);
  CIOVirt one(desc1);
  CDescObject desc4(g_cfg_virt);
  CIOVirt four(desc4);
  io_sdata_t<uint32_t, 1, 4> src4;
  io_sdata_t<uint32_t, 1, 8> src8;
  io_sdata_t<uint32_t, 1, 4> rdata;
  size_t i;

  for (i = 0; i < 4; i++)
    {
      src4(0, i) = 5 + i;
    }
  for (i = 0; i < 8; i++)
    {
      src8(0, i) = 10 + i;
    }

  TEST_ASSERT_EQUAL(OK, one.init());
  TEST_ASSERT_EQUAL(OK, one.initialize(1, 1));
  TEST_ASSERT_EQUAL(OK, one.setData(src4));
  TEST_ASSERT_EQUAL(OK, one.getData(rdata, 1));
  TEST_ASSERT_EQUAL(5, rdata(0, 0));

  TEST_ASSERT_EQUAL(OK, four.init());
  TEST_ASSERT_EQUAL(OK, four.initialize(1, 4));
  TEST_ASSERT_EQUAL(OK, four.setData(src8));
  TEST_ASSERT_EQUAL(OK, four.getData(rdata, 4));
  for (i = 0; i < 4; i++)
    {
      TEST_ASSERT_EQUAL(10 + i, rdata(0, i));
    }
}

//***************************************************************************
// Description: setVal/getVal on a multi-batch virtio move the whole batch
// block in storage order.
//***************************************************************************

static void test_io_virt_multibatch_setval_getval()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1, 4> rdata;
  uint32_t in[4] = {1, 2, 3, 4};
  uint32_t out[4] = {0, 0, 0, 0};
  size_t i;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 4));

  TEST_ASSERT_EQUAL(OK, virt.setVal(in, sizeof(in)));
  TEST_ASSERT_EQUAL(OK, virt.getVal(out, sizeof(out)));
  TEST_ASSERT_EQUAL(OK, virt.getData(rdata, 4));

  for (i = 0; i < 4; i++)
    {
      TEST_ASSERT_EQUAL(in[i], out[i]);
      TEST_ASSERT_EQUAL(in[i], rdata(0, i));
    }
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
// Description: a multi-batch virtio hands the notifier a buffer holding the
// whole batch.
//***************************************************************************

static void test_io_virt_notify_batch_size()
{
  CDescObject desc(g_cfg_virt);
  CIOVirt virt(desc);
  io_sdata_t<uint32_t, 1, 4> data;

  g_notify_batch = 0;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 4, true));
  TEST_ASSERT_EQUAL(OK, virt.setNotifier(virt_notifier_batch, 0, &virt));

  TEST_ASSERT_EQUAL(OK, virt.setData(data));
  TEST_ASSERT_EQUAL(4, g_notify_batch);
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
    DAWN_RUN_TEST(test_io_virt_multibatch_getdata);
    DAWN_RUN_TEST(test_io_virt_multibatch_setdata_rejects_short_batch);
    DAWN_RUN_TEST(test_io_virt_setdata_longer_source_stores_first);
    DAWN_RUN_TEST(test_io_virt_multibatch_setval_getval);

    DAWN_RUN_TEST(test_io_virt_ts_setdata_updates_value);
    DAWN_RUN_TEST(test_io_virt_ts_batched_getdata);
    DAWN_RUN_TEST(test_io_virt_ts_multibatch_roundtrip);

    DAWN_RUN_TEST(test_io_virt_notify_single);
    DAWN_RUN_TEST(test_io_virt_notify_multiple_callbacks);
    DAWN_RUN_TEST(test_io_virt_notify_isolation);
    DAWN_RUN_TEST(test_io_virt_notify_batch_size);
    DAWN_RUN_TEST(test_io_virt_notify_unregister);
    DAWN_RUN_TEST(test_io_virt_notify_set_notifier_null_unregisters);

    DAWN_RUN_TEST(test_io_virt_set_callback);
    DAWN_RUN_TEST(test_io_virt_get_callback);

    return UNITY_END();
  }
}
