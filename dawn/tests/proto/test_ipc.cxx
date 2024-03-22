// dawn/tests/proto/test_ipc.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>

#include "dawn/io/dummy.hxx"
#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/proto/ipc/simple.hxx"
#include "test_common.hxx"
#include "test_simple_proto_common.hxx"

using namespace dawn;

static constexpr auto IPC_RX_PATH = "/var/pipe/ipcrx0";
static constexpr auto IPC_TX_PATH = "/var/pipe/ipctx0";

static constexpr uint8_t CMD_PING = 0x00;
static constexpr uint8_t CMD_PONG = 0x01;
static constexpr uint8_t CMD_GET_IO = 0x10;
static constexpr uint8_t CMD_SET_IO = 0x11;
static constexpr uint8_t CMD_LIST_IOS = 0x21;
static constexpr uint8_t CMD_SUBSCRIBE = 0x30;
static constexpr uint8_t CMD_UNSUB = 0x31;
static constexpr uint8_t CMD_ERROR = 0xFF;
static constexpr uint8_t STATUS_OK = 0x00;
static constexpr uint8_t STATUS_INVALIDOBJ = 0x02;

// IOs read-write

static constexpr auto IPC_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto IPC_DUMMYIO2 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 2);
static constexpr auto IPC_DUMMYIO3 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 3);

// IOs read only with notify

static constexpr auto IPC_NOTIFYIO1 = CIODummyNotify::objectId(SObjectId::DTYPE_UINT32, false, 0);

static int g_ipc_cmd_fd = -1;
static int g_ipc_rsp_fd = -1;

static uint32_t g_cfg_dummy1[] = {
  IPC_DUMMYIO1,
  0,
};

static uint32_t g_cfg_dummy2[] = {
  IPC_DUMMYIO2,
  0,
};

static uint32_t g_cfg_dummy3[] = {
  IPC_DUMMYIO3,
  0,
};

static uint32_t g_cfg_notify1[] = {
  IPC_NOTIFYIO1,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

//***************************************************************************
// Description: minimal IPC descriptor with one IO bound (used by tests
// that exercise dummy1 only).
//***************************************************************************

static uint32_t g_bin_ipc_one_io[] = {
  CProtoIpc::objectId(0),
  3,

  CProtoIpc::cfgIdIOBind(1),
  IPC_DUMMYIO1,

  CProtoIpc::cfgIdRxPath(5),
  0x7261762f,
  0x7069702f,
  0x70692f65,
  0x30787263,
  0x00000000,

  CProtoIpc::cfgIdTxPath(5),
  0x7261762f,
  0x7069702f,
  0x70692f65,
  0x30787463,
  0x00000000,
};

//***************************************************************************
// Description: 3-IO IPC descriptor (used by the LIST_IOS test).
//***************************************************************************

static uint32_t g_bin_ipc_three_ios[] = {
  CProtoIpc::objectId(0),
  3,

  CProtoIpc::cfgIdIOBind(3),
  IPC_DUMMYIO1,
  IPC_DUMMYIO2,
  IPC_DUMMYIO3,

  CProtoIpc::cfgIdRxPath(5),
  0x7261762f,
  0x7069702f,
  0x70692f65,
  0x30787263,
  0x00000000,

  CProtoIpc::cfgIdTxPath(5),
  0x7261762f,
  0x7069702f,
  0x70692f65,
  0x30787463,
  0x00000000,
};

#ifdef CONFIG_DAWN_IO_NOTIFY
//***************************************************************************
// Description: notify-capable descriptor with one notifiable IO bound.
//***************************************************************************

static uint32_t g_bin_ipc_notify[] = {
  CProtoIpc::objectId(0),
  3,

  CProtoIpc::cfgIdIOBind(1),
  IPC_NOTIFYIO1,

  CProtoIpc::cfgIdRxPath(5),
  0x7261762f,
  0x7069702f,
  0x70692f65,
  0x30787263,
  0x00000000,

  CProtoIpc::cfgIdTxPath(5),
  0x7261762f,
  0x7069702f,
  0x70692f65,
  0x30787463,
  0x00000000,
};
#endif

static int read_exact(int fd, uint8_t *buffer, size_t len)
{
  ssize_t ret;
  size_t total = 0;

  while (total < len)
    {
      ret = read(fd, &buffer[total], len - total);
      if (ret < 0)
        {
          return -1;
        }
      if (ret == 0)
        {
          continue;
        }
      total += (size_t)ret;
    }

  return 0;
}

static void send_frame(uint8_t cmd, const uint8_t *payload, size_t len)
{
  uint8_t frame[TEST_SIMPLE_PROTO_FRAME_MIN_LEN + TEST_SIMPLE_PROTO_FRAME_MAX_PAYLOAD];
  size_t frame_len;
  ssize_t ret;
  size_t total = 0;

  frame_len = test_simple_proto_build_frame(cmd, payload, len, frame);

  while (total < frame_len)
    {
      ret = write(g_ipc_cmd_fd, &frame[total], frame_len - total);
      TEST_ASSERT(ret >= 0);
      total += (size_t)ret;
    }
  usleep(1000);
}

static int recv_frame(uint8_t *cmd, uint8_t *payload, size_t *len)
{
  uint8_t frame[TEST_SIMPLE_PROTO_FRAME_MIN_LEN + TEST_SIMPLE_PROTO_FRAME_MAX_PAYLOAD];
  uint16_t payload_len;
  size_t frame_len;
  int ret;
  struct pollfd fds[1];

  std::memset(fds, 0, sizeof(fds));
  fds[0].fd = g_ipc_rsp_fd;
  fds[0].events = POLLIN;

  ret = poll(fds, 1, 100);
  if (ret <= 0)
    {
      return -1;
    }

  ret = read_exact(g_ipc_rsp_fd, &frame[0], 3);
  if (ret < 0)
    {
      return -1;
    }

  payload_len = (uint16_t)(frame[1] | (frame[2] << 8));
  frame_len = TEST_SIMPLE_PROTO_FRAME_MIN_LEN + payload_len;

  ret = read_exact(g_ipc_rsp_fd, &frame[3], frame_len - 3);
  if (ret < 0)
    {
      return -1;
    }

  return test_simple_proto_parse_frame(frame, frame_len, cmd, payload, len);
}

static void cleanup_fifos()
{
  unlink(IPC_RX_PATH);
  unlink(IPC_TX_PATH);
}

static void open_client_fifos()
{
  g_ipc_cmd_fd = open(IPC_RX_PATH, O_RDWR);
  TEST_ASSERT(g_ipc_cmd_fd >= 0);

  g_ipc_rsp_fd = open(IPC_TX_PATH, O_RDWR);
  TEST_ASSERT(g_ipc_rsp_fd >= 0);
}

static void close_client_fifos()
{
  if (g_ipc_cmd_fd >= 0)
    {
      close(g_ipc_cmd_fd);
      g_ipc_cmd_fd = -1;
    }
  if (g_ipc_rsp_fd >= 0)
    {
      close(g_ipc_rsp_fd);
      g_ipc_rsp_fd = -1;
    }
}

static void ipc_pack_le32(uint8_t *buf, uint32_t v)
{
  buf[0] = (uint8_t)(v & 0xFF);
  buf[1] = (uint8_t)((v >> 8) & 0xFF);
  buf[2] = (uint8_t)((v >> 16) & 0xFF);
  buf[3] = (uint8_t)((v >> 24) & 0xFF);
}

//***************************************************************************
// Test cases
//***************************************************************************

//***************************************************************************
// Description: ipc proto runs through start -> hasThread -> stop.
//***************************************************************************

static void test_proto_ipc_lifecycle()
{
  CDescObject descv(g_cfg_dummy1);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_ipc_one_io);
  CProtoIpc ipc(desc);

  cleanup_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  ipc.setObjectMapItem(IPC_DUMMYIO1, &dummy);

  TEST_ASSERT_EQUAL(false, ipc.hasThread());
  TEST_ASSERT_EQUAL(OK, ipc.init());
  open_client_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.start());
  TEST_ASSERT_EQUAL(true, ipc.hasThread());
  TEST_ASSERT_EQUAL(OK, ipc.stop());
  TEST_ASSERT_EQUAL(false, ipc.hasThread());

  close_client_fifos();
  cleanup_fifos();
}

//***************************************************************************
// Description: PING returns a PONG with no payload.
//***************************************************************************

static void test_proto_ipc_ping()
{
  CDescObject descv(g_cfg_dummy1);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_ipc_one_io);
  CProtoIpc ipc(desc);
  uint8_t payload[256];
  size_t len;
  uint8_t cmd;

  cleanup_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  ipc.setObjectMapItem(IPC_DUMMYIO1, &dummy);
  TEST_ASSERT_EQUAL(OK, ipc.init());
  open_client_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.start());

  send_frame(CMD_PING, nullptr, 0);
  TEST_ASSERT_EQUAL(0, recv_frame(&cmd, payload, &len));
  TEST_ASSERT_EQUAL(CMD_PONG, cmd);
  TEST_ASSERT_EQUAL(0, len);

  TEST_ASSERT_EQUAL(OK, ipc.stop());
  close_client_fifos();
  cleanup_fifos();
}

//***************************************************************************
// Description: LIST_IOS returns the count of bound IOs (3).
//***************************************************************************

static void test_proto_ipc_list_ios()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_ipc_three_ios);
  CProtoIpc ipc(desc);
  uint8_t payload[256];
  size_t len;
  uint8_t cmd;

  cleanup_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  TEST_ASSERT_EQUAL(OK, dummy2.configure());
  TEST_ASSERT_EQUAL(OK, dummy2.init());
  TEST_ASSERT_EQUAL(OK, dummy3.configure());
  TEST_ASSERT_EQUAL(OK, dummy3.init());
  ipc.setObjectMapItem(IPC_DUMMYIO1, &dummy1);
  ipc.setObjectMapItem(IPC_DUMMYIO2, &dummy2);
  ipc.setObjectMapItem(IPC_DUMMYIO3, &dummy3);
  TEST_ASSERT_EQUAL(OK, ipc.init());
  open_client_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.start());

  send_frame(CMD_LIST_IOS, nullptr, 0);
  TEST_ASSERT_EQUAL(0, recv_frame(&cmd, payload, &len));
  TEST_ASSERT_EQUAL(CMD_LIST_IOS, cmd);
  TEST_ASSERT_EQUAL(14, len);
  TEST_ASSERT_EQUAL(3, payload[0] | (payload[1] << 8));

  TEST_ASSERT_EQUAL(OK, ipc.stop());
  close_client_fifos();
  cleanup_fifos();
}

//***************************************************************************
// Description: SET_IO followed by GET_IO returns the just-written value.
//***************************************************************************

static void test_proto_ipc_set_then_get_roundtrip()
{
  CDescObject descv(g_cfg_dummy1);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_ipc_one_io);
  CProtoIpc ipc(desc);
  uint8_t request[8];
  uint8_t payload[256];
  size_t len;
  uint8_t cmd;
  uint32_t datau32;

  cleanup_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  ipc.setObjectMapItem(IPC_DUMMYIO1, &dummy);
  TEST_ASSERT_EQUAL(OK, ipc.init());
  open_client_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.start());

  ipc_pack_le32(&request[0], IPC_DUMMYIO1);
  ipc_pack_le32(&request[4], 0x12345678);
  send_frame(CMD_SET_IO, request, 8);
  TEST_ASSERT_EQUAL(0, recv_frame(&cmd, payload, &len));
  TEST_ASSERT_EQUAL(CMD_SET_IO, cmd);
  TEST_ASSERT_EQUAL(STATUS_OK, payload[0]);

  send_frame(CMD_GET_IO, request, 4);
  TEST_ASSERT_EQUAL(0, recv_frame(&cmd, payload, &len));
  TEST_ASSERT_EQUAL(CMD_GET_IO, cmd);
  TEST_ASSERT_EQUAL(4, len);
  std::memcpy(&datau32, &payload[0], sizeof(datau32));
  TEST_ASSERT_EQUAL(0x12345678, datau32);

  TEST_ASSERT_EQUAL(OK, ipc.stop());
  close_client_fifos();
  cleanup_fifos();
}

#ifdef CONFIG_DAWN_IO_NOTIFY
//***************************************************************************
// Description: SUBSCRIBE on an unknown object id returns ERROR with
// STATUS_INVALIDOBJ.
//***************************************************************************

static void test_proto_ipc_subscribe_invalid_objid()
{
  CDescObject descn1(g_cfg_notify1);
  CIODummyNotify notify1(descn1);
  CIONotifier notifier;
  CDescObject desc(g_bin_ipc_notify);
  CProtoIpc ipc(desc);
  uint8_t request[4];
  uint8_t payload[256];
  size_t len;
  uint8_t cmd;

  cleanup_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.configure());
  TEST_ASSERT_EQUAL(OK, notify1.configure());
  TEST_ASSERT_EQUAL(OK, notify1.init());
  notify1.bindNotifier(&notifier);
  ipc.setObjectMapItem(IPC_NOTIFYIO1, &notify1);
  TEST_ASSERT_EQUAL(OK, ipc.init());
  open_client_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.start());

  ipc_pack_le32(request, 0xFFFFFFFF);
  send_frame(CMD_SUBSCRIBE, request, 4);
  TEST_ASSERT_EQUAL(0, recv_frame(&cmd, payload, &len));
  TEST_ASSERT_EQUAL(CMD_ERROR, cmd);
  TEST_ASSERT_EQUAL(2, len);
  TEST_ASSERT_EQUAL(STATUS_INVALIDOBJ, payload[0]);

  TEST_ASSERT_EQUAL(OK, ipc.stop());
  close_client_fifos();
  cleanup_fifos();
}

//***************************************************************************
// Description: SUBSCRIBE on a notifiable IO returns STATUS_OK.
//***************************************************************************

static void test_proto_ipc_subscribe_valid()
{
  CDescObject descn1(g_cfg_notify1);
  CIODummyNotify notify1(descn1);
  CIONotifier notifier;
  CDescObject desc(g_bin_ipc_notify);
  CProtoIpc ipc(desc);
  uint8_t request[4];
  uint8_t payload[256];
  size_t len;
  uint8_t cmd;

  cleanup_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.configure());
  TEST_ASSERT_EQUAL(OK, notify1.configure());
  TEST_ASSERT_EQUAL(OK, notify1.init());
  notify1.bindNotifier(&notifier);
  ipc.setObjectMapItem(IPC_NOTIFYIO1, &notify1);
  TEST_ASSERT_EQUAL(OK, ipc.init());
  open_client_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.start());

  ipc_pack_le32(request, IPC_NOTIFYIO1);
  send_frame(CMD_SUBSCRIBE, request, 4);
  TEST_ASSERT_EQUAL(0, recv_frame(&cmd, payload, &len));
  TEST_ASSERT_EQUAL(CMD_SUBSCRIBE, cmd);
  TEST_ASSERT_EQUAL(STATUS_OK, payload[0]);

  TEST_ASSERT_EQUAL(OK, ipc.stop());
  close_client_fifos();
  cleanup_fifos();
}

//***************************************************************************
// Description: UNSUBSCRIBE returns STATUS_OK after a prior SUBSCRIBE.
//***************************************************************************

static void test_proto_ipc_unsubscribe()
{
  CDescObject descn1(g_cfg_notify1);
  CIODummyNotify notify1(descn1);
  CIONotifier notifier;
  CDescObject desc(g_bin_ipc_notify);
  CProtoIpc ipc(desc);
  uint8_t request[4];
  uint8_t payload[256];
  size_t len;
  uint8_t cmd;

  cleanup_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.configure());
  TEST_ASSERT_EQUAL(OK, notify1.configure());
  TEST_ASSERT_EQUAL(OK, notify1.init());
  notify1.bindNotifier(&notifier);
  ipc.setObjectMapItem(IPC_NOTIFYIO1, &notify1);
  TEST_ASSERT_EQUAL(OK, ipc.init());
  open_client_fifos();
  TEST_ASSERT_EQUAL(OK, ipc.start());

  ipc_pack_le32(request, IPC_NOTIFYIO1);
  send_frame(CMD_SUBSCRIBE, request, 4);
  TEST_ASSERT_EQUAL(0, recv_frame(&cmd, payload, &len));
  TEST_ASSERT_EQUAL(CMD_SUBSCRIBE, cmd);
  TEST_ASSERT_EQUAL(STATUS_OK, payload[0]);

  send_frame(CMD_UNSUB, request, 4);
  TEST_ASSERT_EQUAL(0, recv_frame(&cmd, payload, &len));
  TEST_ASSERT_EQUAL(CMD_UNSUB, cmd);
  TEST_ASSERT_EQUAL(STATUS_OK, payload[0]);

  TEST_ASSERT_EQUAL(OK, ipc.stop());
  close_client_fifos();
  cleanup_fifos();
}
#endif

extern "C"
{
  int test_proto_ipc()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_proto_ipc_lifecycle);
    DAWN_RUN_TEST(test_proto_ipc_ping);
    DAWN_RUN_TEST(test_proto_ipc_list_ios);
    DAWN_RUN_TEST(test_proto_ipc_set_then_get_roundtrip);

#ifdef CONFIG_DAWN_IO_NOTIFY
    DAWN_RUN_TEST(test_proto_ipc_subscribe_invalid_objid);
    DAWN_RUN_TEST(test_proto_ipc_subscribe_valid);
    DAWN_RUN_TEST(test_proto_ipc_unsubscribe);
#endif

    return UNITY_END();
  }
}
