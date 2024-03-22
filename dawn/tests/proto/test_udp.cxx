// dawn/tests/proto/test_udp.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

#include "dawn/io/dummy.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/proto/udp/simple.hxx"
#include "test_common.hxx"
#include "test_simple_proto_common.hxx"

using namespace dawn;

static constexpr auto TEST_UDP_PORT = 50005;

static constexpr uint8_t CMD_PING = 0x00;
static constexpr uint8_t CMD_PONG = 0x01;
static constexpr uint8_t CMD_GET_IO = 0x10;
static constexpr uint8_t CMD_SET_IO = 0x11;
static constexpr uint8_t CMD_LIST_IOS = 0x21;
static constexpr uint8_t STATUS_OK = 0x00;

// IOs read-write

static constexpr auto UDP_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto UDP_DUMMYIO2 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 2);
static constexpr auto UDP_DUMMYIO3 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 3);

static int g_udp_fd = -1;
static struct sockaddr_in g_server_addr;

static uint32_t g_cfg_dummy1[] = {
  UDP_DUMMYIO1,
  0,
};

static uint32_t g_cfg_dummy2[] = {
  UDP_DUMMYIO2,
  0,
};

static uint32_t g_cfg_dummy3[] = {
  UDP_DUMMYIO3,
  0,
};

//***************************************************************************
// Description: minimal UDP descriptor with one IO bound (used by tests
// that only exercise dummy1).
//***************************************************************************

static uint32_t g_bin_udp_one_io[] = {
  CProtoUdp::objectId(0),
  2,
  CProtoUdp::cfgIdIOBind(1),
  UDP_DUMMYIO1,
  CProtoUdp::cfgIdPort(),
  TEST_UDP_PORT,
};

//***************************************************************************
// Description: 3-IO UDP descriptor (used by the LIST_IOS test).
//***************************************************************************

static uint32_t g_bin_udp_three_ios[] = {
  CProtoUdp::objectId(0),
  2,
  CProtoUdp::cfgIdIOBind(3),
  UDP_DUMMYIO1,
  UDP_DUMMYIO2,
  UDP_DUMMYIO3,
  CProtoUdp::cfgIdPort(),
  TEST_UDP_PORT,
};

//***************************************************************************
// Helper Functions
//***************************************************************************

static void send_frame(uint8_t cmd, const uint8_t *payload, size_t len)
{
  uint8_t frame[TEST_SIMPLE_PROTO_FRAME_MIN_LEN + TEST_SIMPLE_PROTO_FRAME_MAX_PAYLOAD];
  size_t frame_len;
  ssize_t ret;

  frame_len = test_simple_proto_build_frame(cmd, payload, len, frame);

  ret = sendto(g_udp_fd,
               frame,
               frame_len,
               0,
               reinterpret_cast<struct sockaddr *>(&g_server_addr),
               sizeof(g_server_addr));
  TEST_ASSERT(ret == (ssize_t)frame_len);

  usleep(1000);
}

static int recv_frame(uint8_t *cmd, uint8_t *payload, size_t *len)
{
  uint8_t buffer[TEST_SIMPLE_PROTO_FRAME_MIN_LEN + TEST_SIMPLE_PROTO_FRAME_MAX_PAYLOAD];
  ssize_t ret;
  struct pollfd fds[1];

  fds[0].fd = g_udp_fd;
  fds[0].events = POLLIN;

  ret = poll(fds, 1, 100);
  if (ret <= 0)
    {
      return -1;
    }

  ret = recvfrom(g_udp_fd, buffer, sizeof(buffer), 0, NULL, NULL);
  if (ret < TEST_SIMPLE_PROTO_FRAME_MIN_LEN)
    {
      return -1;
    }

  return test_simple_proto_parse_frame(buffer, (size_t)ret, cmd, payload, len);
}

static void udp_pack_le32(uint8_t *buf, uint32_t v)
{
  buf[0] = (uint8_t)(v & 0xFF);
  buf[1] = (uint8_t)((v >> 8) & 0xFF);
  buf[2] = (uint8_t)((v >> 16) & 0xFF);
  buf[3] = (uint8_t)((v >> 24) & 0xFF);
}

//***************************************************************************
// Description: udp proto runs through start -> hasThread -> stop.
//***************************************************************************

static void test_proto_udp_lifecycle()
{
  CDescObject descv(g_cfg_dummy1);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_udp_one_io);
  CProtoUdp udp(desc);

  TEST_ASSERT_EQUAL(OK, udp.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  udp.setObjectMapItem(UDP_DUMMYIO1, &dummy);

  TEST_ASSERT_EQUAL(false, udp.hasThread());
  TEST_ASSERT_EQUAL(OK, udp.init());
  TEST_ASSERT_EQUAL(OK, udp.start());
  TEST_ASSERT_EQUAL(true, udp.hasThread());
  TEST_ASSERT_EQUAL(OK, udp.stop());
  TEST_ASSERT_EQUAL(false, udp.hasThread());
}

//***************************************************************************
// Description: PING returns a PONG with no payload.
//***************************************************************************

static void test_proto_udp_ping()
{
  CDescObject descv(g_cfg_dummy1);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_udp_one_io);
  CProtoUdp udp(desc);
  uint8_t payload[256];
  size_t len;
  uint8_t cmd;

  TEST_ASSERT_EQUAL(OK, udp.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  udp.setObjectMapItem(UDP_DUMMYIO1, &dummy);
  TEST_ASSERT_EQUAL(OK, udp.init());
  TEST_ASSERT_EQUAL(OK, udp.start());

  send_frame(CMD_PING, nullptr, 0);
  TEST_ASSERT_EQUAL(0, recv_frame(&cmd, payload, &len));
  TEST_ASSERT_EQUAL(CMD_PONG, cmd);
  TEST_ASSERT_EQUAL(0, len);

  TEST_ASSERT_EQUAL(OK, udp.stop());
}

//***************************************************************************
// Description: LIST_IOS returns the count of bound IOs (3) followed by
// their object ids.
//***************************************************************************

static void test_proto_udp_list_ios()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_udp_three_ios);
  CProtoUdp udp(desc);
  uint8_t payload[256];
  size_t len;
  uint8_t cmd;

  TEST_ASSERT_EQUAL(OK, udp.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  TEST_ASSERT_EQUAL(OK, dummy2.configure());
  TEST_ASSERT_EQUAL(OK, dummy2.init());
  TEST_ASSERT_EQUAL(OK, dummy3.configure());
  TEST_ASSERT_EQUAL(OK, dummy3.init());
  udp.setObjectMapItem(UDP_DUMMYIO1, &dummy1);
  udp.setObjectMapItem(UDP_DUMMYIO2, &dummy2);
  udp.setObjectMapItem(UDP_DUMMYIO3, &dummy3);
  TEST_ASSERT_EQUAL(OK, udp.init());
  TEST_ASSERT_EQUAL(OK, udp.start());

  send_frame(CMD_LIST_IOS, nullptr, 0);
  TEST_ASSERT_EQUAL(0, recv_frame(&cmd, payload, &len));
  TEST_ASSERT_EQUAL(CMD_LIST_IOS, cmd);
  TEST_ASSERT_EQUAL(14, len);
  TEST_ASSERT_EQUAL(3, payload[0] | (payload[1] << 8));

  TEST_ASSERT_EQUAL(OK, udp.stop());
}

//***************************************************************************
// Description: SET_IO followed by GET_IO returns the just-written value.
//***************************************************************************

static void test_proto_udp_set_then_get_roundtrip()
{
  CDescObject descv(g_cfg_dummy1);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_udp_one_io);
  CProtoUdp udp(desc);
  uint8_t request[8];
  uint8_t payload[256];
  size_t len;
  uint8_t cmd;
  uint32_t data;

  TEST_ASSERT_EQUAL(OK, udp.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  udp.setObjectMapItem(UDP_DUMMYIO1, &dummy);
  TEST_ASSERT_EQUAL(OK, udp.init());
  TEST_ASSERT_EQUAL(OK, udp.start());

  udp_pack_le32(&request[0], UDP_DUMMYIO1);
  udp_pack_le32(&request[4], 0x12345678);
  send_frame(CMD_SET_IO, request, 8);
  TEST_ASSERT_EQUAL(0, recv_frame(&cmd, payload, &len));
  TEST_ASSERT_EQUAL(CMD_SET_IO, cmd);
  TEST_ASSERT_EQUAL(1, len);
  TEST_ASSERT_EQUAL(STATUS_OK, payload[0]);

  send_frame(CMD_GET_IO, request, 4);
  TEST_ASSERT_EQUAL(0, recv_frame(&cmd, payload, &len));
  TEST_ASSERT_EQUAL(CMD_GET_IO, cmd);
  TEST_ASSERT_EQUAL(4, len);
  std::memcpy(&data, &payload[0], sizeof(data));
  TEST_ASSERT_EQUAL(0x12345678, data);

  TEST_ASSERT_EQUAL(OK, udp.stop());
}

extern "C"
{
  int test_proto_udp()
  {
    UNITY_BEGIN();

    g_udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    TEST_ASSERT(g_udp_fd > 0);

    std::memset(&g_server_addr, 0, sizeof(g_server_addr));
    g_server_addr.sin_family = AF_INET;
    g_server_addr.sin_port = htons(TEST_UDP_PORT);
    g_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    DAWN_RUN_TEST(test_proto_udp_lifecycle);
    DAWN_RUN_TEST(test_proto_udp_ping);
    DAWN_RUN_TEST(test_proto_udp_list_ios);
    DAWN_RUN_TEST(test_proto_udp_set_then_get_roundtrip);

    close(g_udp_fd);

    return UNITY_END();
  }
}
