// dawn/tests/prog/test_gateway.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/prog/gateway.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto GW_VIRTIO1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 0);
static constexpr auto GW_VIRTIO2 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 2);
static constexpr auto GW_VIRTIO3 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 3);
static constexpr auto GW_VIRTIO4 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 4);

// VirtIO descriptors

static uint32_t g_cfg_virt1[] = {
  GW_VIRTIO1,
  0,
};

static uint32_t g_cfg_virt2[] = {
  GW_VIRTIO2,
  0,
};

static uint32_t g_cfg_virt3[] = {
  GW_VIRTIO3,
  0,
};

static uint32_t g_cfg_virt4[] = {
  GW_VIRTIO4,
  0,
};

// gateway 1 - single binding, io1 write -> io2

static uint32_t g_bin_gw1[] = {
  CProgGateway::objectId(0),
  1,

  CProgGateway::cfgIdIOBind(4),
  GW_VIRTIO1,
  GW_VIRTIO2,
  CProgGateway::FLAG_IO1_WRITE | CProgGateway::FLAG_IO2_READ,
  1,
};

// gateway 2 - single binding, io2 write -> io1

static uint32_t g_bin_gw2[] = {
  CProgGateway::objectId(2),
  1,

  CProgGateway::cfgIdIOBind(4),
  GW_VIRTIO1,
  GW_VIRTIO2,
  CProgGateway::FLAG_IO2_WRITE | CProgGateway::FLAG_IO1_READ,
  1,
};

// gateway 3 - single binding, fetch io1 from io2

static uint32_t g_bin_gw3[] = {
  CProgGateway::objectId(0),
  1,

  CProgGateway::cfgIdIOBind(4),
  GW_VIRTIO1,
  GW_VIRTIO2,
  CProgGateway::FLAG_IO1_READ,
  1,
};

// gateway 4 - single binding, fetch io2 from io1

static uint32_t g_bin_gw4[] = {
  CProgGateway::objectId(2),
  1,

  CProgGateway::cfgIdIOBind(4),
  GW_VIRTIO1,
  GW_VIRTIO2,
  CProgGateway::FLAG_IO2_READ,
  1,
};

// gateway 5 - two bindings in one config item

static uint32_t g_bin_gw5[] = {
  CProgGateway::objectId(0),
  1,

  CProgGateway::cfgIdIOBind(8),

  // Binding 0: vio1 <-> vio2, io1 write -> io2
  GW_VIRTIO1,
  GW_VIRTIO2,
  CProgGateway::FLAG_IO1_WRITE | CProgGateway::FLAG_IO2_READ,
  1,

  // Binding 1: vio3 <-> vio4, io2 write -> io1
  GW_VIRTIO3,
  GW_VIRTIO4,
  CProgGateway::FLAG_IO2_WRITE | CProgGateway::FLAG_IO1_READ,
  1,
};

//***************************************************************************
// Description: gateway propagates writes from IO1 to IO2.
//***************************************************************************

static void test_prog_gateway_write_io1()
{
  CDescObject desc1(g_cfg_virt1);
  CIOVirt virt1(desc1);
  CDescObject desc2(g_cfg_virt2);
  CIOVirt virt2(desc2);

  CDescObject desc_gw(g_bin_gw1);
  CProgGateway gw(desc_gw);

  io_sdata_t<uint32_t, 1, 1> writeData;
  io_sdata_t<uint32_t, 1, 1> readData;
  int ret;

  TEST_ASSERT_EQUAL(OK, virt1.init());
  TEST_ASSERT_EQUAL(OK, virt2.init());
  TEST_ASSERT_EQUAL(OK, gw.configure());

  gw.setObjectMapItem(GW_VIRTIO1, &virt1);
  gw.setObjectMapItem(GW_VIRTIO2, &virt2);

  TEST_ASSERT_EQUAL(OK, gw.init());

  ret = gw.start();
  TEST_ASSERT_EQUAL(OK, ret);

  // Write to io1, verify propagated to io2

  writeData(0) = 42;
  TEST_ASSERT_EQUAL(OK, virt1.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt2.getData(readData, 1));
  TEST_ASSERT_EQUAL(42, readData(0));

  writeData(0) = 123;
  TEST_ASSERT_EQUAL(OK, virt1.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt2.getData(readData, 1));
  TEST_ASSERT_EQUAL(123, readData(0));

  ret = gw.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: gateway propagates writes from IO2 to IO1.
//***************************************************************************

static void test_prog_gateway_write_io2()
{
  CDescObject desc1(g_cfg_virt1);
  CIOVirt virt1(desc1);
  CDescObject desc2(g_cfg_virt2);
  CIOVirt virt2(desc2);

  CDescObject desc_gw(g_bin_gw2);
  CProgGateway gw(desc_gw);

  io_sdata_t<uint32_t, 1, 1> writeData;
  io_sdata_t<uint32_t, 1, 1> readData;
  int ret;

  TEST_ASSERT_EQUAL(OK, virt1.init());
  TEST_ASSERT_EQUAL(OK, virt2.init());
  TEST_ASSERT_EQUAL(OK, gw.configure());

  gw.setObjectMapItem(GW_VIRTIO1, &virt1);
  gw.setObjectMapItem(GW_VIRTIO2, &virt2);

  TEST_ASSERT_EQUAL(OK, gw.init());

  ret = gw.start();
  TEST_ASSERT_EQUAL(OK, ret);

  // Write to io2, verify propagated to io1

  writeData(0) = 77;
  TEST_ASSERT_EQUAL(OK, virt2.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt1.getData(readData, 1));
  TEST_ASSERT_EQUAL(77, readData(0));

  writeData(0) = 255;
  TEST_ASSERT_EQUAL(OK, virt2.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt1.getData(readData, 1));
  TEST_ASSERT_EQUAL(255, readData(0));

  ret = gw.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: gateway fetches IO1 reads from IO2 on demand.
//***************************************************************************

static void test_prog_gateway_fetch_io1()
{
  CDescObject desc1(g_cfg_virt1);
  CIOVirt virt1(desc1);
  CDescObject desc2(g_cfg_virt2);
  CIOVirt virt2(desc2);

  CDescObject desc_gw(g_bin_gw3);
  CProgGateway gw(desc_gw);

  io_sdata_t<uint32_t, 1, 1> writeData;
  io_sdata_t<uint32_t, 1, 1> readData;
  int ret;

  TEST_ASSERT_EQUAL(OK, virt1.init());
  TEST_ASSERT_EQUAL(OK, virt2.init());
  TEST_ASSERT_EQUAL(OK, gw.configure());

  gw.setObjectMapItem(GW_VIRTIO1, &virt1);
  gw.setObjectMapItem(GW_VIRTIO2, &virt2);

  TEST_ASSERT_EQUAL(OK, gw.init());

  ret = gw.start();
  TEST_ASSERT_EQUAL(OK, ret);

  // Write to io2 directly; reading io1 fetches from io2 on demand

  writeData(0) = 99;
  TEST_ASSERT_EQUAL(OK, virt2.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt1.getData(readData, 1));
  TEST_ASSERT_EQUAL(99, readData(0));

  writeData(0) = 200;
  TEST_ASSERT_EQUAL(OK, virt2.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt1.getData(readData, 1));
  TEST_ASSERT_EQUAL(200, readData(0));

  ret = gw.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: gateway fetches IO2 reads from IO1 on demand.
//***************************************************************************

static void test_prog_gateway_fetch_io2()
{
  CDescObject desc1(g_cfg_virt1);
  CIOVirt virt1(desc1);
  CDescObject desc2(g_cfg_virt2);
  CIOVirt virt2(desc2);

  CDescObject desc_gw(g_bin_gw4);
  CProgGateway gw(desc_gw);

  io_sdata_t<uint32_t, 1, 1> writeData;
  io_sdata_t<uint32_t, 1, 1> readData;
  int ret;

  TEST_ASSERT_EQUAL(OK, virt1.init());
  TEST_ASSERT_EQUAL(OK, virt2.init());
  TEST_ASSERT_EQUAL(OK, gw.configure());

  gw.setObjectMapItem(GW_VIRTIO1, &virt1);
  gw.setObjectMapItem(GW_VIRTIO2, &virt2);

  TEST_ASSERT_EQUAL(OK, gw.init());

  ret = gw.start();
  TEST_ASSERT_EQUAL(OK, ret);

  // Write to io1 directly; reading io2 fetches from io1 on demand

  writeData(0) = 55;
  TEST_ASSERT_EQUAL(OK, virt1.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt2.getData(readData, 1));
  TEST_ASSERT_EQUAL(55, readData(0));

  writeData(0) = 111;
  TEST_ASSERT_EQUAL(OK, virt1.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt2.getData(readData, 1));
  TEST_ASSERT_EQUAL(111, readData(0));

  ret = gw.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: two independent bindings in one gateway instance
//***************************************************************************

static void test_prog_gateway_multi_bind()
{
  CDescObject desc1(g_cfg_virt1);
  CIOVirt virt1(desc1);
  CDescObject desc2(g_cfg_virt2);
  CIOVirt virt2(desc2);
  CDescObject desc3(g_cfg_virt3);
  CIOVirt virt3(desc3);
  CDescObject desc4(g_cfg_virt4);
  CIOVirt virt4(desc4);

  CDescObject desc_gw(g_bin_gw5);
  CProgGateway gw(desc_gw);

  io_sdata_t<uint32_t, 1, 1> writeData;
  io_sdata_t<uint32_t, 1, 1> readData;
  int ret;

  TEST_ASSERT_EQUAL(OK, virt1.init());
  TEST_ASSERT_EQUAL(OK, virt2.init());
  TEST_ASSERT_EQUAL(OK, virt3.init());
  TEST_ASSERT_EQUAL(OK, virt4.init());
  TEST_ASSERT_EQUAL(OK, gw.configure());

  gw.setObjectMapItem(GW_VIRTIO1, &virt1);
  gw.setObjectMapItem(GW_VIRTIO2, &virt2);
  gw.setObjectMapItem(GW_VIRTIO3, &virt3);
  gw.setObjectMapItem(GW_VIRTIO4, &virt4);

  TEST_ASSERT_EQUAL(OK, gw.init());

  ret = gw.start();
  TEST_ASSERT_EQUAL(OK, ret);

  // Binding 0: write vio1, read vio2

  writeData(0) = 10;
  TEST_ASSERT_EQUAL(OK, virt1.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt2.getData(readData, 1));
  TEST_ASSERT_EQUAL(10, readData(0));

  // Binding 1: write vio4, read vio3 (io2 -> io1 direction)

  writeData(0) = 20;
  TEST_ASSERT_EQUAL(OK, virt4.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt3.getData(readData, 1));
  TEST_ASSERT_EQUAL(20, readData(0));

  // Bindings are independent - binding 0 still holds its last value

  TEST_ASSERT_EQUAL(OK, virt2.getData(readData, 1));
  TEST_ASSERT_EQUAL(10, readData(0));

  ret = gw.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: stop clears gateway callbacks and restart installs them again.
//***************************************************************************

static void test_prog_gateway_stop_clears_callbacks()
{
  CDescObject desc1(g_cfg_virt1);
  CIOVirt virt1(desc1);
  CDescObject desc2(g_cfg_virt2);
  CIOVirt virt2(desc2);
  CDescObject desc3(g_cfg_virt3);
  CIOVirt virt3(desc3);
  CDescObject desc4(g_cfg_virt4);
  CIOVirt virt4(desc4);

  CDescObject desc_gw(g_bin_gw5);
  CProgGateway gw(desc_gw);

  io_sdata_t<uint32_t, 1, 1> writeData;
  io_sdata_t<uint32_t, 1, 1> readData;
  int ret;

  TEST_ASSERT_EQUAL(OK, virt1.init());
  TEST_ASSERT_EQUAL(OK, virt2.init());
  TEST_ASSERT_EQUAL(OK, virt3.init());
  TEST_ASSERT_EQUAL(OK, virt4.init());
  TEST_ASSERT_EQUAL(OK, gw.configure());

  gw.setObjectMapItem(GW_VIRTIO1, &virt1);
  gw.setObjectMapItem(GW_VIRTIO2, &virt2);
  gw.setObjectMapItem(GW_VIRTIO3, &virt3);
  gw.setObjectMapItem(GW_VIRTIO4, &virt4);

  TEST_ASSERT_EQUAL(OK, gw.init());
  TEST_ASSERT_EQUAL(OK, gw.start());

  writeData(0) = 10;
  TEST_ASSERT_EQUAL(OK, virt1.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt2.getData(readData, 1));
  TEST_ASSERT_EQUAL(10, readData(0));

  writeData(0) = 20;
  TEST_ASSERT_EQUAL(OK, virt4.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt3.getData(readData, 1));
  TEST_ASSERT_EQUAL(20, readData(0));

  ret = gw.stop();
  TEST_ASSERT_EQUAL(OK, ret);

  writeData(0) = 30;
  TEST_ASSERT_EQUAL(OK, virt1.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt2.getData(readData, 1));
  TEST_ASSERT_EQUAL(10, readData(0));

  writeData(0) = 40;
  TEST_ASSERT_EQUAL(OK, virt4.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt3.getData(readData, 1));
  TEST_ASSERT_EQUAL(20, readData(0));

  TEST_ASSERT_EQUAL(OK, gw.start());

  writeData(0) = 50;
  TEST_ASSERT_EQUAL(OK, virt1.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt2.getData(readData, 1));
  TEST_ASSERT_EQUAL(50, readData(0));

  writeData(0) = 60;
  TEST_ASSERT_EQUAL(OK, virt4.setData(writeData));
  TEST_ASSERT_EQUAL(OK, virt3.getData(readData, 1));
  TEST_ASSERT_EQUAL(60, readData(0));

  ret = gw.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

extern "C"
{
  int test_prog_gateway()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_prog_gateway_write_io1);
    DAWN_RUN_TEST(test_prog_gateway_write_io2);
    DAWN_RUN_TEST(test_prog_gateway_fetch_io1);
    DAWN_RUN_TEST(test_prog_gateway_fetch_io2);
    DAWN_RUN_TEST(test_prog_gateway_multi_bind);
    DAWN_RUN_TEST(test_prog_gateway_stop_clears_callbacks);

    return UNITY_END();
  }
}
