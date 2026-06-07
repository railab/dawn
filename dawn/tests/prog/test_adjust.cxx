// dawn/tests/prog/test_adjust.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>

#include "dawn/io/dummy.hxx"
#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/adjust.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto ADJUST_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto ADJUST_DUMMYIO2 = CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto ADJUST_DUMMYIO3 = CIODummy::objectId(SObjectId::DTYPE_FLOAT, false, 3);
static constexpr auto ADJUST_DUMMYIO4 = CIODummy::objectId(SObjectId::DTYPE_INT16, false, 4);
static constexpr auto ADJUST_DUMMYIO5 = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 5);
static constexpr auto ADJUST_DUMMYIO6 = CIODummyNotify::objectId(SObjectId::DTYPE_UINT32, false, 6);
static constexpr auto ADJUST_DUMMYIO7 = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 7);

static constexpr auto ADJUST_VIRTIO1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 0);
static constexpr auto ADJUST_VIRTIO2 = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 0);
static constexpr auto ADJUST_VIRTIO3 = CIOVirt::objectId(SObjectId::DTYPE_B16, false, 0);
static constexpr auto ADJUST_VIRTIO4 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 4);
static constexpr auto ADJUST_VIRTIO5 = CIOVirt::objectId(SObjectId::DTYPE_UINT64, false, 5);
static constexpr auto ADJUST_VIRTIO6 = CIOVirt::objectId(SObjectId::DTYPE_DOUBLE, false, 6);

// dummy IOs

static uint32_t g_cfg_dummy1[] = {
  ADJUST_DUMMYIO1,
  2,
  CIODummy::cfgIdDim(),
  10,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_INT32, true, 10),
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
};

static uint32_t g_cfg_dummy2[] = {
  ADJUST_DUMMYIO2,
  3,
  CIODummyNotify::cfgIdDim(),
  10,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 10),
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  CIODummyNotify::cfgInterval(false),
  1000,
};

static uint32_t g_cfg_dummy3[] = {
  ADJUST_DUMMYIO3,
  2,
  CIODummy::cfgIdDim(),
  3,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_FLOAT, true, 3),
  SObjectCfg::fToCfg(1.0f),
  SObjectCfg::fToCfg(2.0f),
  SObjectCfg::fToCfg(3.0f),
};

static uint32_t g_cfg_dummy4[] = {
  ADJUST_DUMMYIO4,
  2,
  CIODummy::cfgIdDim(),
  3,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_INT16, true, 3),
  1,
  2,
  3,
};

static uint32_t g_cfg_dummy5[] = {
  ADJUST_DUMMYIO5,
  2,
  CIODummy::cfgIdDim(),
  10,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 10),
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
};

static uint32_t g_cfg_dummy6[] = {
  ADJUST_DUMMYIO6,
  3,
  CIODummyNotify::cfgIdDim(),
  1,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  1000,
};

static uint32_t g_cfg_dummy7[] = {
  ADJUST_DUMMYIO7,
  2,
  CIODummy::cfgIdDim(),
  1,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 1),
  0,
};

static uint32_t g_cfg_virt1[] = {
  ADJUST_VIRTIO1,
  0,
};

static uint32_t g_cfg_virt2[] = {
  ADJUST_VIRTIO2,
  0,
};

static uint32_t g_cfg_virt3[] = {
  ADJUST_VIRTIO3,
  0,
};

static uint32_t g_cfg_virt4[] = {
  ADJUST_VIRTIO4,
  0,
};

static uint32_t g_cfg_virt5[] = {
  ADJUST_VIRTIO5,
  0,
};

static uint32_t g_cfg_virt6[] = {
  ADJUST_VIRTIO6,
  0,
};

// prog adjust 1 - uint32_t output (fetch only, identity transform)

uint32_t g_bin_adjust1[] = {
  CProgAdjust::objectId(0),
  1,

  CProgAdjust::cfgIdIOBind(),
  ADJUST_DUMMYIO1,
  ADJUST_VIRTIO1,
};

// prog adjust 2 - float output (fetch only, x = 2 * y + 3)

uint32_t g_bin_adjust2[] = {
  CProgAdjust::objectId(2),
  2,

  CProgAdjust::cfgIdIOBind(),
  ADJUST_DUMMYIO1,
  ADJUST_VIRTIO2,

  CProgAdjust::cfgParams(),
  SObjectCfg::fToCfg(3.0f), // Offset
  SObjectCfg::fToCfg(2.0f)  // Scale
};

// prog adjust with runtime-writable params (cfgParams rw=true), float
// output (fetch only, initial x = 2 * y + 3).

static uint32_t g_bin_adjust_rwparams[] = {
  CProgAdjust::objectId(8),
  2,

  CProgAdjust::cfgIdIOBind(),
  ADJUST_DUMMYIO1,
  ADJUST_VIRTIO2,

  CProgAdjust::cfgParams(true), // rw -> writable at runtime
  SObjectCfg::fToCfg(3.0f),     // Offset
  SObjectCfg::fToCfg(2.0f)      // Scale
};

// prog adjust 3 - b16_t output (fetch only, x = 2 * y + 3)

uint32_t g_bin_adjust3[] = {
  CProgAdjust::objectId(3),
  2,

  CProgAdjust::cfgIdIOBind(),
  ADJUST_DUMMYIO1,
  ADJUST_VIRTIO3,

  CProgAdjust::cfgParams(),
  SObjectCfg::fToB16ToCfg(3.0f), // Offset
  SObjectCfg::fToB16ToCfg(2.0f)  // Scale
};

// prog adjust 4 - uint32_t output (notifier supported, identity)

uint32_t g_bin_adjust4[] = {
  CProgAdjust::objectId(0),
  1,

  CProgAdjust::cfgIdIOBind(),
  ADJUST_DUMMYIO2,
  ADJUST_VIRTIO1,
};

// prog adjust 5 - float output (notifier supported, x = 2 * y + 3)

uint32_t g_bin_adjust5[] = {
  CProgAdjust::objectId(2),
  2,

  CProgAdjust::cfgIdIOBind(),
  ADJUST_DUMMYIO2,
  ADJUST_VIRTIO2,

  CProgAdjust::cfgParams(),
  SObjectCfg::fToCfg(3.0f), // Offset
  SObjectCfg::fToCfg(2.0f)  // Scale
};

// prog adjust 6 - b16_t output (notifier supported, x = 2 * y + 3)

uint32_t g_bin_adjust6[] = {
  CProgAdjust::objectId(3),
  2,

  CProgAdjust::cfgIdIOBind(),
  ADJUST_DUMMYIO2,
  ADJUST_VIRTIO3,

  CProgAdjust::cfgParams(),
  SObjectCfg::fToB16ToCfg(3.0f), // Offset
  SObjectCfg::fToB16ToCfg(2.0f)  // Scale
};

// prog adjust 7 - float source to int32_t output (fetch only, x = 2 * y + 3)

uint32_t g_bin_adjust7[] = {
  CProgAdjust::objectId(4),
  2,

  CProgAdjust::cfgIdIOBind(),
  ADJUST_DUMMYIO3,
  ADJUST_VIRTIO4,

  CProgAdjust::cfgParams(),
  3,
  2,
};

// prog adjust 8 - int16 source to uint64_t output (fetch only, x = 2 * y + 3)

uint32_t g_bin_adjust8[] = {
  CProgAdjust::objectId(5),
  2,

  CProgAdjust::cfgIdIOBind(),
  ADJUST_DUMMYIO4,
  ADJUST_VIRTIO5,

  CProgAdjust::cfgParams(),
  3,
  2,
};

// prog adjust 9 - int16 source to double output (fetch only, x = 1.5 * y + 0.5)

uint32_t g_bin_adjust9[] = {
  CProgAdjust::objectId(6),
  2,

  CProgAdjust::cfgIdIOBind(),
  ADJUST_DUMMYIO4,
  ADJUST_VIRTIO6,

  CProgAdjust::cfgParams(),
  SObjectCfg::fToCfg(0.5f), // Offset
  SObjectCfg::fToCfg(1.5f)  // Scale
};

// prog adjust 10 - generic dummy output (fetch only, identity transform)

uint32_t g_bin_adjust10[] = {
  CProgAdjust::objectId(7),
  1,

  CProgAdjust::cfgIdIOBind(),
  ADJUST_DUMMYIO1,
  ADJUST_DUMMYIO5,
};

// prog adjust 11 - write-through source to generic dummy output (x = 2 * y + 3)

uint32_t g_bin_adjust11[] = {
  CProgAdjust::objectId(8),
  2,

  CProgAdjust::cfgIdIOBind(),
  ADJUST_DUMMYIO6,
  ADJUST_DUMMYIO7,

  CProgAdjust::cfgParams(),
  3,
  2,
};

//***************************************************************************
// Notifier callbacks
//
// Each callback validates a single dtype's adjust output. The source dummy
// is initialised to 0..9, so identity (adj1) yields 0..9, and the scaled
// programs (off=3, scale=2) yield 3, 5, 7, ... 21.
//***************************************************************************

static int adj_vio_callback_uint32(void *priv, io_ddata_t *d)
{
  DAWNASSERT(d != nullptr, "nullptr pointer");

  uint32_t *data = (uint32_t *)d->getDataPtr();

  TEST_ASSERT_EQUAL(0, data[0]);
  TEST_ASSERT_EQUAL(1, data[1]);
  TEST_ASSERT_EQUAL(2, data[2]);
  TEST_ASSERT_EQUAL(3, data[3]);
  TEST_ASSERT_EQUAL(4, data[4]);
  TEST_ASSERT_EQUAL(5, data[5]);
  TEST_ASSERT_EQUAL(6, data[6]);
  TEST_ASSERT_EQUAL(7, data[7]);
  TEST_ASSERT_EQUAL(8, data[8]);
  TEST_ASSERT_EQUAL(9, data[9]);

  return OK;
}

static int adj_vio_callback_float(void *priv, io_ddata_t *d)
{
  DAWNASSERT(d != nullptr, "nullptr pointer");

  float *data = (float *)d->getDataPtr();

  TEST_ASSERT_EQUAL(3.0f, data[0]);
  TEST_ASSERT_EQUAL(5.0f, data[1]);
  TEST_ASSERT_EQUAL(7.0f, data[2]);
  TEST_ASSERT_EQUAL(9.0f, data[3]);
  TEST_ASSERT_EQUAL(11.0f, data[4]);
  TEST_ASSERT_EQUAL(13.0f, data[5]);
  TEST_ASSERT_EQUAL(15.0f, data[6]);
  TEST_ASSERT_EQUAL(17.0f, data[7]);
  TEST_ASSERT_EQUAL(19.0f, data[8]);
  TEST_ASSERT_EQUAL(21.0f, data[9]);

  return OK;
}

static int adj_vio_callback_b16(void *priv, io_ddata_t *d)
{
  DAWNASSERT(d != nullptr, "nullptr pointer");

  b16_t *data = (b16_t *)d->getDataPtr();

  TEST_ASSERT_EQUAL(ftob16(3.0f), data[0]);
  TEST_ASSERT_EQUAL(ftob16(5.0f), data[1]);
  TEST_ASSERT_EQUAL(ftob16(7.0f), data[2]);
  TEST_ASSERT_EQUAL(ftob16(9.0f), data[3]);
  TEST_ASSERT_EQUAL(ftob16(11.0f), data[4]);
  TEST_ASSERT_EQUAL(ftob16(13.0f), data[5]);
  TEST_ASSERT_EQUAL(ftob16(15.0f), data[6]);
  TEST_ASSERT_EQUAL(ftob16(17.0f), data[7]);
  TEST_ASSERT_EQUAL(ftob16(19.0f), data[8]);
  TEST_ASSERT_EQUAL(ftob16(21.0f), data[9]);

  return OK;
}

// Configure + init + bind a fetch-only chain (CIODummy -> output -> adjust).
// Caller is responsible for adj.start()/stop().

static void adjust_setup_fetch_chain(CIODummy &src,
                                     uint32_t srcId,
                                     CIOCommon &output,
                                     uint32_t outputId,
                                     CProgAdjust &adj)
{
  TEST_ASSERT_EQUAL(OK, src.configure());
  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, output.configure());
  TEST_ASSERT_EQUAL(OK, output.init());
  TEST_ASSERT_EQUAL(OK, adj.configure());
  adj.setObjectMapItem(srcId, &src);
  adj.setObjectMapItem(outputId, &output);
  TEST_ASSERT_EQUAL(OK, adj.init());
}

// Configure + init + bind a notify chain (CIODummyNotify -> output ->
// adjust) and wire up the supplied notifier.  Caller is responsible for
// adj.start()/stop() and notifier/src start()/stop().

static void adjust_setup_notify_chain(CIODummyNotify &src,
                                      uint32_t srcId,
                                      CIOCommon &output,
                                      uint32_t outputId,
                                      CProgAdjust &adj,
                                      CIONotifier &notifier)
{
  TEST_ASSERT_EQUAL(OK, src.configure());
  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, output.configure());
  TEST_ASSERT_EQUAL(OK, output.init());
  TEST_ASSERT_EQUAL(OK, adj.configure());
  src.bindNotifier(&notifier);
  adj.setObjectMapItem(srcId, &src);
  adj.setObjectMapItem(outputId, &output);
  TEST_ASSERT_EQUAL(OK, adj.init());
}

//***************************************************************************
// Description: fetch-only adjust with identity transform on uint32 output.
//***************************************************************************

static void test_prog_adjust_fetch_uint32_identity()
{
  CDescObject descSrc(g_cfg_dummy1);
  CIODummy src(descSrc);
  CDescObject descVio(g_cfg_virt1);
  CIOVirt vio(descVio);
  CDescObject descAdj(g_bin_adjust1);
  CProgAdjust adj(descAdj);
  io_sdata_t<uint32_t, 10, 1> data;

  adjust_setup_fetch_chain(src, ADJUST_DUMMYIO1, vio, ADJUST_VIRTIO1, adj);
  TEST_ASSERT_EQUAL(OK, adj.start());

  // Source dummy is initialised to 0..9; identity transform => 0..9.

  TEST_ASSERT_EQUAL(OK, vio.getData(data, 1));
  TEST_ASSERT_EQUAL(0, data(0));
  TEST_ASSERT_EQUAL(1, data(1));
  TEST_ASSERT_EQUAL(2, data(2));
  TEST_ASSERT_EQUAL(3, data(3));
  TEST_ASSERT_EQUAL(4, data(4));
  TEST_ASSERT_EQUAL(5, data(5));
  TEST_ASSERT_EQUAL(6, data(6));
  TEST_ASSERT_EQUAL(7, data(7));
  TEST_ASSERT_EQUAL(8, data(8));
  TEST_ASSERT_EQUAL(9, data(9));

  TEST_ASSERT_EQUAL(OK, adj.stop());
}

//***************************************************************************
// Description: fetch-only adjust with x = 2y + 3 transform on float output.
//***************************************************************************

static void test_prog_adjust_fetch_float_scale_offset()
{
  CDescObject descSrc(g_cfg_dummy1);
  CIODummy src(descSrc);
  CDescObject descVio(g_cfg_virt2);
  CIOVirt vio(descVio);
  CDescObject descAdj(g_bin_adjust2);
  CProgAdjust adj(descAdj);
  io_sdata_t<float, 10, 1> data;

  adjust_setup_fetch_chain(src, ADJUST_DUMMYIO1, vio, ADJUST_VIRTIO2, adj);
  TEST_ASSERT_EQUAL(OK, adj.start());

  // Source 0..9, scale=2, offset=3 => 3, 5, 7, ... 21.

  TEST_ASSERT_EQUAL(OK, vio.getData(data, 1));
  TEST_ASSERT_EQUAL(3.0f, data(0));
  TEST_ASSERT_EQUAL(5.0f, data(1));
  TEST_ASSERT_EQUAL(7.0f, data(2));
  TEST_ASSERT_EQUAL(9.0f, data(3));
  TEST_ASSERT_EQUAL(11.0f, data(4));
  TEST_ASSERT_EQUAL(13.0f, data(5));
  TEST_ASSERT_EQUAL(15.0f, data(6));
  TEST_ASSERT_EQUAL(17.0f, data(7));
  TEST_ASSERT_EQUAL(19.0f, data(8));
  TEST_ASSERT_EQUAL(21.0f, data(9));

  TEST_ASSERT_EQUAL(OK, adj.stop());
}

//***************************************************************************
// Description: fetch-only adjust with x = 2y + 3 transform on b16 output.
//***************************************************************************

static void test_prog_adjust_fetch_b16_scale_offset()
{
  CDescObject descSrc(g_cfg_dummy1);
  CIODummy src(descSrc);
  CDescObject descVio(g_cfg_virt3);
  CIOVirt vio(descVio);
  CDescObject descAdj(g_bin_adjust3);
  CProgAdjust adj(descAdj);
  io_sdata_t<b16_t, 10, 1> data;

  adjust_setup_fetch_chain(src, ADJUST_DUMMYIO1, vio, ADJUST_VIRTIO3, adj);
  TEST_ASSERT_EQUAL(OK, adj.start());

  // Source 0..9, scale=2, offset=3 => ftob16(3, 5, ... 21).

  TEST_ASSERT_EQUAL(OK, vio.getData(data, 1));
  TEST_ASSERT_EQUAL(ftob16(3.0f), data(0));
  TEST_ASSERT_EQUAL(ftob16(5.0f), data(1));
  TEST_ASSERT_EQUAL(ftob16(7.0f), data(2));
  TEST_ASSERT_EQUAL(ftob16(9.0f), data(3));
  TEST_ASSERT_EQUAL(ftob16(11.0f), data(4));
  TEST_ASSERT_EQUAL(ftob16(13.0f), data(5));
  TEST_ASSERT_EQUAL(ftob16(15.0f), data(6));
  TEST_ASSERT_EQUAL(ftob16(17.0f), data(7));
  TEST_ASSERT_EQUAL(ftob16(19.0f), data(8));
  TEST_ASSERT_EQUAL(ftob16(21.0f), data(9));

  TEST_ASSERT_EQUAL(OK, adj.stop());
}

//***************************************************************************
// Description: fetch-only adjust supports float source conversion to int32.
//***************************************************************************

static void test_prog_adjust_fetch_float_to_int32_scale_offset()
{
  CDescObject descSrc(g_cfg_dummy3);
  CIODummy src(descSrc);
  CDescObject descVio(g_cfg_virt4);
  CIOVirt vio(descVio);
  CDescObject descAdj(g_bin_adjust7);
  CProgAdjust adj(descAdj);
  io_sdata_t<int32_t, 3, 1> data;

  adjust_setup_fetch_chain(src, ADJUST_DUMMYIO3, vio, ADJUST_VIRTIO4, adj);
  TEST_ASSERT_EQUAL(OK, adj.start());

  TEST_ASSERT_EQUAL(OK, vio.getData(data, 1));
  TEST_ASSERT_EQUAL(5, data(0));
  TEST_ASSERT_EQUAL(7, data(1));
  TEST_ASSERT_EQUAL(9, data(2));

  TEST_ASSERT_EQUAL(OK, adj.stop());
}

//***************************************************************************
// Description: fetch-only adjust supports narrow integer source to uint64.
//***************************************************************************

static void test_prog_adjust_fetch_int16_to_uint64_scale_offset()
{
  CDescObject descSrc(g_cfg_dummy4);
  CIODummy src(descSrc);
  CDescObject descVio(g_cfg_virt5);
  CIOVirt vio(descVio);
  CDescObject descAdj(g_bin_adjust8);
  CProgAdjust adj(descAdj);
  io_sdata_t<uint64_t, 3, 1> data;

  adjust_setup_fetch_chain(src, ADJUST_DUMMYIO4, vio, ADJUST_VIRTIO5, adj);
  TEST_ASSERT_EQUAL(OK, adj.start());

  TEST_ASSERT_EQUAL(OK, vio.getData(data, 1));
  TEST_ASSERT_EQUAL_UINT64(5, data(0));
  TEST_ASSERT_EQUAL_UINT64(7, data(1));
  TEST_ASSERT_EQUAL_UINT64(9, data(2));

  TEST_ASSERT_EQUAL(OK, adj.stop());
}

//***************************************************************************
// Description: fetch-only adjust supports narrow integer source to double.
//***************************************************************************

static void test_prog_adjust_fetch_int16_to_double_scale_offset()
{
  CDescObject descSrc(g_cfg_dummy4);
  CIODummy src(descSrc);
  CDescObject descVio(g_cfg_virt6);
  CIOVirt vio(descVio);
  CDescObject descAdj(g_bin_adjust9);
  CProgAdjust adj(descAdj);
  io_sdata_t<double, 3, 1> data;

  adjust_setup_fetch_chain(src, ADJUST_DUMMYIO4, vio, ADJUST_VIRTIO6, adj);
  TEST_ASSERT_EQUAL(OK, adj.start());

  TEST_ASSERT_EQUAL(OK, vio.getData(data, 1));
  TEST_ASSERT_EQUAL_FLOAT(2.0f, static_cast<float>(data(0)));
  TEST_ASSERT_EQUAL_FLOAT(3.5f, static_cast<float>(data(1)));
  TEST_ASSERT_EQUAL_FLOAT(5.0f, static_cast<float>(data(2)));

  TEST_ASSERT_EQUAL(OK, adj.stop());
}

//***************************************************************************
// Description: fetch-only adjust can write to a generic CIOCommon output.
//***************************************************************************

static void test_prog_adjust_fetch_generic_output()
{
  CDescObject descSrc(g_cfg_dummy1);
  CIODummy src(descSrc);
  CDescObject descOut(g_cfg_dummy5);
  CIODummy out(descOut);
  CDescObject descAdj(g_bin_adjust10);
  CProgAdjust adj(descAdj);
  io_sdata_t<uint32_t, 10, 1> data;

  adjust_setup_fetch_chain(src, ADJUST_DUMMYIO1, out, ADJUST_DUMMYIO5, adj);
  TEST_ASSERT_EQUAL(OK, adj.start());

  TEST_ASSERT_EQUAL(OK, out.getData(data, 1));
  TEST_ASSERT_EQUAL(0, data(0));
  TEST_ASSERT_EQUAL(1, data(1));
  TEST_ASSERT_EQUAL(2, data(2));
  TEST_ASSERT_EQUAL(3, data(3));
  TEST_ASSERT_EQUAL(4, data(4));
  TEST_ASSERT_EQUAL(5, data(5));
  TEST_ASSERT_EQUAL(6, data(6));
  TEST_ASSERT_EQUAL(7, data(7));
  TEST_ASSERT_EQUAL(8, data(8));
  TEST_ASSERT_EQUAL(9, data(9));

  TEST_ASSERT_EQUAL(OK, adj.stop());
}

//***************************************************************************
// Description: adjust can pass writes through a notify-capable input IO.
//***************************************************************************

static void test_prog_adjust_write_through_generic_output()
{
  CDescObject descSrc(g_cfg_dummy6);
  CIODummyNotify src(descSrc);
  CDescObject descOut(g_cfg_dummy7);
  CIODummy out(descOut);
  CDescObject descAdj(g_bin_adjust11);
  CProgAdjust adj(descAdj);
  CIONotifier notifier;
  io_sdata_t<uint32_t, 1, 1> srcData;
  io_sdata_t<uint32_t, 1, 1> outData;

  adjust_setup_notify_chain(src, ADJUST_DUMMYIO6, out, ADJUST_DUMMYIO7, adj, notifier);
  TEST_ASSERT_EQUAL(OK, adj.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_EQUAL(OK, src.start());

  srcData(0, 0) = 10;
  TEST_ASSERT_EQUAL(OK, src.setData(srcData));
  usleep(2000);

  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(23, outData(0));

  srcData(0, 0) = 4;
  TEST_ASSERT_EQUAL(OK, src.setData(srcData));
  usleep(2000);

  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(11, outData(0));

  TEST_ASSERT_EQUAL(OK, src.stop());
  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, adj.stop());
}

//***************************************************************************
// Description: notify-driven adjust with identity transform on uint32; the
// vio callback validates the output.
//***************************************************************************

static void test_prog_adjust_notify_uint32_identity()
{
  CDescObject descSrc(g_cfg_dummy2);
  CIODummyNotify src(descSrc);
  CDescObject descVio(g_cfg_virt1);
  CIOVirt vio(descVio);
  CDescObject descAdj(g_bin_adjust4);
  CProgAdjust adj(descAdj);
  CIONotifier notifier;
  io_sdata_t<int32_t, 10, 1> src_data;
  size_t i;

  adjust_setup_notify_chain(src, ADJUST_DUMMYIO2, vio, ADJUST_VIRTIO1, adj, notifier);

  TEST_ASSERT_EQUAL(OK, vio.setNotifier(adj_vio_callback_uint32, 0, &vio));
  TEST_ASSERT_EQUAL(OK, adj.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_EQUAL(OK, src.start());

  for (i = 0; i < 10; i++)
    {
      src_data(i, 0) = i;
    }
  TEST_ASSERT_EQUAL(OK, src.setData(src_data));
  usleep(2000);

  TEST_ASSERT_EQUAL(OK, src.stop());
  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, adj.stop());
}

//***************************************************************************
// Description: notify-driven adjust with x = 2y + 3 transform on float; the
// vio callback validates the output.
//***************************************************************************

static void test_prog_adjust_notify_float_scale_offset()
{
  CDescObject descSrc(g_cfg_dummy2);
  CIODummyNotify src(descSrc);
  CDescObject descVio(g_cfg_virt2);
  CIOVirt vio(descVio);
  CDescObject descAdj(g_bin_adjust5);
  CProgAdjust adj(descAdj);
  CIONotifier notifier;
  io_sdata_t<int32_t, 10, 1> src_data;
  size_t i;

  adjust_setup_notify_chain(src, ADJUST_DUMMYIO2, vio, ADJUST_VIRTIO2, adj, notifier);

  TEST_ASSERT_EQUAL(OK, vio.setNotifier(adj_vio_callback_float, 0, &vio));
  TEST_ASSERT_EQUAL(OK, adj.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_EQUAL(OK, src.start());

  for (i = 0; i < 10; i++)
    {
      src_data(i, 0) = i;
    }
  TEST_ASSERT_EQUAL(OK, src.setData(src_data));
  usleep(2000);

  TEST_ASSERT_EQUAL(OK, src.stop());
  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, adj.stop());
}

//***************************************************************************
// Description: notify-driven adjust with x = 2y + 3 transform on b16; the
// vio callback validates the output.
//***************************************************************************

static void test_prog_adjust_notify_b16_scale_offset()
{
  CDescObject descSrc(g_cfg_dummy2);
  CIODummyNotify src(descSrc);
  CDescObject descVio(g_cfg_virt3);
  CIOVirt vio(descVio);
  CDescObject descAdj(g_bin_adjust6);
  CProgAdjust adj(descAdj);
  CIONotifier notifier;
  io_sdata_t<int32_t, 10, 1> src_data;
  size_t i;

  adjust_setup_notify_chain(src, ADJUST_DUMMYIO2, vio, ADJUST_VIRTIO3, adj, notifier);

  TEST_ASSERT_EQUAL(OK, vio.setNotifier(adj_vio_callback_b16, 0, &vio));
  TEST_ASSERT_EQUAL(OK, adj.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_EQUAL(OK, src.start());

  for (i = 0; i < 10; i++)
    {
      src_data(i, 0) = i;
    }
  TEST_ASSERT_EQUAL(OK, src.setData(src_data));
  usleep(2000);

  TEST_ASSERT_EQUAL(OK, src.stop());
  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, adj.stop());
}

//***************************************************************************
// Description: rw params (cfgParams rw=true) can be rewritten at runtime via
// setObjConfig; onSetObjConfig refreshes the cached scale/offset so the next
// computation uses them. A write to the read-only params id is rejected.
//***************************************************************************

static void test_prog_adjust_runtime_params_update()
{
  CDescObject descSrc(g_cfg_dummy1);
  CIODummy src(descSrc);
  CDescObject descVio(g_cfg_virt2);
  CIOVirt vio(descVio);
  CDescObject descAdj(g_bin_adjust_rwparams);
  CProgAdjust adj(descAdj);
  io_sdata_t<float, 10, 1> data;
  uint32_t params[2];

  adjust_setup_fetch_chain(src, ADJUST_DUMMYIO1, vio, ADJUST_VIRTIO2, adj);
  TEST_ASSERT_EQUAL(OK, adj.start());

  // Initial params give out = 2 * in + 3 (src is 0..9).
  TEST_ASSERT_EQUAL(OK, vio.getData(data, 1));
  TEST_ASSERT_EQUAL(3.0f, data(0));
  TEST_ASSERT_EQUAL(5.0f, data(1));

  params[0] = SObjectCfg::fToCfg(10.0f); // Offset
  params[1] = SObjectCfg::fToCfg(1.0f);  // Scale

  // A write to the read-only params id (rw=false) is denied.
  TEST_ASSERT_EQUAL(-EACCES, adj.setObjConfig(CProgAdjust::cfgParams(false), params, 2));

  // Writing the rw params id refreshes the cached scale/offset.
  TEST_ASSERT_EQUAL(OK, adj.setObjConfig(CProgAdjust::cfgParams(true), params, 2));

  // Recompute: the new params give out = 1 * in + 10.
  TEST_ASSERT_EQUAL(OK, adj.stop());
  TEST_ASSERT_EQUAL(OK, adj.start());
  TEST_ASSERT_EQUAL(OK, vio.getData(data, 1));
  TEST_ASSERT_EQUAL(10.0f, data(0));
  TEST_ASSERT_EQUAL(11.0f, data(1));

  TEST_ASSERT_EQUAL(OK, adj.stop());
}

extern "C"
{
  int test_prog_adjust()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_prog_adjust_fetch_uint32_identity);
    DAWN_RUN_TEST(test_prog_adjust_fetch_float_scale_offset);
    DAWN_RUN_TEST(test_prog_adjust_fetch_b16_scale_offset);
    DAWN_RUN_TEST(test_prog_adjust_fetch_float_to_int32_scale_offset);
    DAWN_RUN_TEST(test_prog_adjust_fetch_int16_to_uint64_scale_offset);
    DAWN_RUN_TEST(test_prog_adjust_fetch_int16_to_double_scale_offset);
    DAWN_RUN_TEST(test_prog_adjust_fetch_generic_output);
    DAWN_RUN_TEST(test_prog_adjust_write_through_generic_output);

    DAWN_RUN_TEST(test_prog_adjust_notify_uint32_identity);
    DAWN_RUN_TEST(test_prog_adjust_notify_float_scale_offset);
    DAWN_RUN_TEST(test_prog_adjust_notify_b16_scale_offset);

    DAWN_RUN_TEST(test_prog_adjust_runtime_params_update);

    return UNITY_END();
  }
}
