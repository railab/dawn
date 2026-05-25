// dawn/tests/proto/test_modbus_tcp.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <cstring>

#include "dawn/io/dummy.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/proto/modbus/tcp.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto MB_COIL_GET = 0x1;
static constexpr auto MB_DISCRETE_GET = 0x2;
static constexpr auto MB_HOLDING_GET = 0x3;
static constexpr auto MB_COIL_SET = 0x5;
static constexpr auto MB_HOLDING_SET = 0x6;

static constexpr auto MODBUS_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_BOOL, false, 0);
static constexpr auto MODBUS_DUMMYIO4 = CIODummy::objectId(SObjectId::DTYPE_UINT16, false, 4);

static constexpr auto REGS_COILS1 = 0x01;
static constexpr auto REGS_HOLDING1 = 0x3000;

static uint32_t g_cfg_dummy1[] = {
  MODBUS_DUMMYIO1,
  0,
};

static uint32_t g_cfg_dummy4[] = {
  MODBUS_DUMMYIO4,
  0,
};

static uint32_t g_bin_modbus_tcp_coil[] = {
  CProtoModbusTcp::objectId(0),
  2,

  CProtoModbusTcp::cfgIdPort(),
  15020,

  CProtoModbusTcp::cfgIdIOBind(5),
  CProtoModbusRegs::MODBUS_TYPE_COIL,
  0,
  REGS_COILS1,
  1,
  MODBUS_DUMMYIO1,
};

static uint32_t g_bin_modbus_tcp_holding[] = {
  CProtoModbusTcp::objectId(0),
  2,

  CProtoModbusTcp::cfgIdPort(),
  15021,

  CProtoModbusTcp::cfgIdIOBind(5),
  CProtoModbusRegs::MODBUS_TYPE_HOLDING,
  0,
  REGS_HOLDING1,
  1,
  MODBUS_DUMMYIO4,
};

static uint32_t g_bin_modbus_tcp_exception[] = {
  CProtoModbusTcp::objectId(0),
  2,

  CProtoModbusTcp::cfgIdPort(),
  15022,

  CProtoModbusTcp::cfgIdIOBind(5),
  CProtoModbusRegs::MODBUS_TYPE_COIL,
  0,
  REGS_COILS1,
  1,
  MODBUS_DUMMYIO1,
};

static int mbtcp_connect(uint16_t port)
{
  struct sockaddr_in addr;
  int fd;
  int ret;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    {
      return fd;
    }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  ret = connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
  if (ret < 0)
    {
      close(fd);
      return ret;
    }

  return fd;
}

static int mbtcp_recv_full(int fd, uint8_t *buffer, size_t len, int timeout_ms)
{
  struct timespec start;
  struct timespec now;
  size_t off;
  int ret;
  int elapsed_ms;

  off = 0;
  elapsed_ms = 0;

  clock_gettime(CLOCK_MONOTONIC, &start);

  while (off < len)
    {
      ret = recv(fd, buffer + off, len - off, 0);
      if (ret > 0)
        {
          off += (size_t)ret;
          continue;
        }

      clock_gettime(CLOCK_MONOTONIC, &now);
      elapsed_ms =
        (int)((now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000);
      if (elapsed_ms >= timeout_ms)
        {
          return (ret == 0) ? 0 : -1;
        }

      if (errno == EINTR)
        {
          continue;
        }

      if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          usleep(1000);
          continue;
        }

      return -1;
    }

  return (int)off;
}

static int mbtcp_send_read_req(int fd,
                               uint16_t tid,
                               uint8_t unit,
                               uint8_t func,
                               uint16_t addr,
                               uint16_t qty)
{
  uint8_t req[12];

  req[0] = (uint8_t)(tid >> 8);
  req[1] = (uint8_t)(tid & 0xff);
  req[2] = 0;
  req[3] = 0;
  req[4] = 0;
  req[5] = 6;
  req[6] = unit;
  req[7] = func;
  req[8] = (uint8_t)(addr >> 8);
  req[9] = (uint8_t)(addr & 0xff);
  req[10] = (uint8_t)(qty >> 8);
  req[11] = (uint8_t)(qty & 0xff);

  return send(fd, req, sizeof(req), 0);
}

static int mbtcp_send_write_single_req(int fd,
                                       uint16_t tid,
                                       uint8_t unit,
                                       uint8_t func,
                                       uint16_t addr,
                                       uint16_t value)
{
  return mbtcp_send_read_req(fd, tid, unit, func, addr, value);
}

static int mbtcp_recv_adu(int fd, uint8_t *buffer, size_t buflen)
{
  int ret;
  uint16_t len;
  size_t need;
  size_t off;

  if (buflen < 9)
    {
      return -1;
    }

  ret = mbtcp_recv_full(fd, buffer, 7, 5000);
  if (ret != 7)
    {
      return ret;
    }

  len = ((uint16_t)buffer[4] << 8) | buffer[5];
  if (len < 1)
    {
      return -1;
    }

  need = (size_t)len - 1;
  if (7 + need > buflen)
    {
      return -1;
    }

  ret = mbtcp_recv_full(fd, buffer + 7, need, 5000);
  if (ret <= 0)
    {
      return ret;
    }

  off = 7 + (size_t)ret;

  return (int)off;
}

// Configure + init dummy and modbus, bind, start, connect a TCP client to
// the supplied port.  Caller closes fd and stops modbus.

static int mbtcp_setup(CIODummy &dummy, uint32_t io_id, CProtoModbusTcp &modbus, uint16_t port)
{
  int fd;

  TEST_ASSERT_EQUAL(OK, modbus.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  modbus.setObjectMapItem(io_id, &dummy);
  TEST_ASSERT_EQUAL(OK, modbus.init());
  TEST_ASSERT_EQUAL(OK, modbus.start());
  usleep(100000);

  fd = mbtcp_connect(port);
  TEST_ASSERT(fd > 0);
  return fd;
}

static void test_proto_modbus_tcp_not_listening_before_start()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_tcp_coil);
  CProtoModbusTcp modbus(desc);
  int fd;

  TEST_ASSERT_EQUAL(OK, modbus.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  modbus.setObjectMapItem(MODBUS_DUMMYIO1, &dummy1);
  TEST_ASSERT_EQUAL(OK, modbus.init());

  fd = mbtcp_connect(15020);
  TEST_ASSERT_TRUE(fd < 0);
}

//***************************************************************************
// Description: reading a single coil at its initial value returns one
// status byte = 0.
//***************************************************************************

static void test_proto_modbus_tcp_coil_read_initial()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_tcp_coil);
  CProtoModbusTcp modbus(desc);
  uint8_t buffer[64];
  int fd;
  int ret;

  fd = mbtcp_setup(dummy1, MODBUS_DUMMYIO1, modbus, 15020);

  ret = mbtcp_send_read_req(fd, 1, CONFIG_DAWN_PROTO_MODBUS_TCP_ADDR, MB_COIL_GET, REGS_COILS1, 1);
  TEST_ASSERT_EQUAL(12, ret);

  ret = mbtcp_recv_adu(fd, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL(10, ret);
  TEST_ASSERT_EQUAL(CONFIG_DAWN_PROTO_MODBUS_TCP_ADDR, buffer[6]);
  TEST_ASSERT_EQUAL(MB_COIL_GET, buffer[7]);
  TEST_ASSERT_EQUAL(1, buffer[8]);
  TEST_ASSERT_EQUAL(0, buffer[9]);

  close(fd);
  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: writing a coil with 0xff00 echoes the request and a
// subsequent read sees the new value (1).
//***************************************************************************

static void test_proto_modbus_tcp_coil_write_then_read()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_tcp_coil);
  CProtoModbusTcp modbus(desc);
  io_sdata_t<bool, 1> data;
  uint8_t buffer[64];
  int fd;
  int ret;

  fd = mbtcp_setup(dummy1, MODBUS_DUMMYIO1, modbus, 15020);

  ret = mbtcp_send_write_single_req(
    fd, 1, CONFIG_DAWN_PROTO_MODBUS_TCP_ADDR, MB_COIL_SET, REGS_COILS1, 0xff00);
  TEST_ASSERT_EQUAL(12, ret);
  ret = mbtcp_recv_adu(fd, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL(12, ret);
  TEST_ASSERT_EQUAL(MB_COIL_SET, buffer[7]);

  ret = mbtcp_send_read_req(fd, 2, CONFIG_DAWN_PROTO_MODBUS_TCP_ADDR, MB_COIL_GET, REGS_COILS1, 1);
  TEST_ASSERT_EQUAL(12, ret);
  ret = mbtcp_recv_adu(fd, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL(10, ret);
  TEST_ASSERT_EQUAL(1, buffer[9]);

  TEST_ASSERT_EQUAL(OK, dummy1.getData(data, 1));
  TEST_ASSERT_EQUAL(1, data(0));

  close(fd);
  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading a single holding register at its initial value
// returns two zero bytes.
//***************************************************************************

static void test_proto_modbus_tcp_holding_read_initial()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject desc(g_bin_modbus_tcp_holding);
  CProtoModbusTcp modbus(desc);
  uint8_t buffer[64];
  int fd;
  int ret;

  fd = mbtcp_setup(dummy4, MODBUS_DUMMYIO4, modbus, 15021);

  ret =
    mbtcp_send_read_req(fd, 1, CONFIG_DAWN_PROTO_MODBUS_TCP_ADDR, MB_HOLDING_GET, REGS_HOLDING1, 1);
  TEST_ASSERT_EQUAL(12, ret);

  ret = mbtcp_recv_adu(fd, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL(11, ret);
  TEST_ASSERT_EQUAL(MB_HOLDING_GET, buffer[7]);
  TEST_ASSERT_EQUAL(2, buffer[8]);

  close(fd);
  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: writing a holding register echoes the value and the bound
// dummy reflects the new contents.
//***************************************************************************

static void test_proto_modbus_tcp_holding_write_then_read()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject desc(g_bin_modbus_tcp_holding);
  CProtoModbusTcp modbus(desc);
  io_sdata_t<uint16_t, 1> data;
  uint8_t buffer[64];
  int fd;
  int ret;

  fd = mbtcp_setup(dummy4, MODBUS_DUMMYIO4, modbus, 15021);

  ret = mbtcp_send_write_single_req(
    fd, 1, CONFIG_DAWN_PROTO_MODBUS_TCP_ADDR, MB_HOLDING_SET, REGS_HOLDING1, 0x1234);
  TEST_ASSERT_EQUAL(12, ret);
  ret = mbtcp_recv_adu(fd, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL(12, ret);
  TEST_ASSERT_EQUAL(MB_HOLDING_SET, buffer[7]);
  TEST_ASSERT_EQUAL(0x12, buffer[10]);
  TEST_ASSERT_EQUAL(0x34, buffer[11]);

  TEST_ASSERT_EQUAL(OK, dummy4.getData(data, 1));
  TEST_ASSERT_EQUAL(0x1234, data(0));

  close(fd);
  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: a discrete-read on a coil-only proto returns an exception
// 0x02 (illegal data address).
//***************************************************************************

static void test_proto_modbus_tcp_exception_no_register()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_tcp_exception);
  CProtoModbusTcp modbus(desc);
  uint8_t buffer[64];
  int fd;
  int ret;

  fd = mbtcp_setup(dummy1, MODBUS_DUMMYIO1, modbus, 15022);

  ret = mbtcp_send_read_req(fd, 1, CONFIG_DAWN_PROTO_MODBUS_TCP_ADDR, MB_DISCRETE_GET, 0, 1);
  TEST_ASSERT_EQUAL(12, ret);

  ret = mbtcp_recv_adu(fd, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL(9, ret);
  TEST_ASSERT_EQUAL(CONFIG_DAWN_PROTO_MODBUS_TCP_ADDR, buffer[6]);
  TEST_ASSERT_EQUAL(0x80 + MB_DISCRETE_GET, buffer[7]);
  TEST_ASSERT_EQUAL(0x02, buffer[8]);

  close(fd);
  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

extern "C"
{
  int test_proto_modbus_tcp()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_proto_modbus_tcp_not_listening_before_start);
    DAWN_RUN_TEST(test_proto_modbus_tcp_coil_read_initial);
    DAWN_RUN_TEST(test_proto_modbus_tcp_coil_write_then_read);
    DAWN_RUN_TEST(test_proto_modbus_tcp_holding_read_initial);
    DAWN_RUN_TEST(test_proto_modbus_tcp_holding_write_then_read);
    DAWN_RUN_TEST(test_proto_modbus_tcp_exception_no_register);

    return UNITY_END();
  }
}
