// dawn/tests/proto/test_serial.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <cstring>

#include "dawn/io/dummy.hxx"
#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/proto/serial/simple.hxx"
#include "mocks/fake_ioseekmock.hxx"
#include "test_common.hxx"
#include "test_simple_proto_common.hxx"

using namespace dawn;

static constexpr auto SERIAL_PIPE_PATH = "/dev/pipe0";

// IOs read-write

static constexpr auto SERIAL_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto SERIAL_DUMMYIO2 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 2);
static constexpr auto SERIAL_DUMMYIO3 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 3);
static constexpr auto SERIAL_SEEKIO = CIODummy::objectId(SObjectId::DTYPE_UINT8, false, 4);

// IOs read only with notify

static constexpr auto SERIAL_NOTIFYIO1 =
  CIODummyNotify::objectId(SObjectId::DTYPE_UINT32, false, 0);
static constexpr auto SERIAL_NOTIFYIO2 =
  CIODummyNotify::objectId(SObjectId::DTYPE_UINT32, false, 2);

// Simple-protocol command opcodes used by the tests.

static constexpr uint8_t CMD_GET_IO = 0x10;
static constexpr uint8_t CMD_SET_IO = 0x11;
static constexpr uint8_t CMD_GET_IO_SEEK = 0x14;
static constexpr uint8_t CMD_SET_IO_SEEK = 0x15;
static constexpr uint8_t CMD_GET_INFO = 0x20;
static constexpr uint8_t CMD_LIST_IOS = 0x21;
static constexpr uint8_t CMD_SUBSCRIBE = 0x30;
static constexpr uint8_t CMD_UNSUBSCRIBE = 0x31;
static constexpr uint8_t CMD_ERROR = 0xFF;
static constexpr uint8_t STATUS_OK = 0x00;
static constexpr uint8_t STATUS_INVALID = 0x01;
static constexpr uint8_t STATUS_INVALIDOBJ = 0x02;
static constexpr uint8_t STATUS_ERROR = 0xFF;

// Pty0 used to communicate with programs

static int g_pty_fd;

static void open_test_pty()
{
  int ret;
  struct termios tio;

  g_pty_fd = open("/dev/pty0", O_RDWR);
  TEST_ASSERT(g_pty_fd > 0);

  ret = unlockpt(g_pty_fd);
  TEST_ASSERT(ret == 0);
  dawn_test_drain_pty_master(g_pty_fd);

  tcgetattr(g_pty_fd, &tio);
  cfmakeraw(&tio);
  tcsetattr(g_pty_fd, TCSANOW, &tio);
}

static void close_test_pty()
{
  if (g_pty_fd >= 0)
    {
      dawn_test_drain_pty_master(g_pty_fd);
      close(g_pty_fd);
      g_pty_fd = -1;
    }
}

static uint32_t g_cfg_dummy1[] = {
  SERIAL_DUMMYIO1,
  0,
};

static uint32_t g_cfg_dummy2[] = {
  SERIAL_DUMMYIO2,
  0,
};

static uint32_t g_cfg_dummy3[] = {
  SERIAL_DUMMYIO3,
  0,
};

static uint32_t g_cfg_notify1[] = {
  SERIAL_NOTIFYIO1,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

static uint32_t g_cfg_notify2[] = {
  SERIAL_NOTIFYIO2,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

static uint32_t g_cfg_seekio[] = {
  SERIAL_SEEKIO,
  0,
};

//***************************************************************************
// Description: Serial protocol descriptor
//***************************************************************************

static uint32_t g_bin_serial_simple[] = {
  // Object ID

  CProtoSerial::objectId(0),
  3,

  // Allocated objects

  CProtoSerial::cfgIdIOBind(4),
  SERIAL_DUMMYIO1,
  SERIAL_DUMMYIO2,
  SERIAL_DUMMYIO3,
  SERIAL_SEEKIO,

  // Path

  CProtoSerial::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Baud rate

  CProtoSerial::cfgIdBaud(),
  115200,
};

static uint32_t g_bin_serial_notify[] = {
  // Object ID

  CProtoSerial::objectId(0),
  3,

  // Allocated objects (including notifiable IOs)

  CProtoSerial::cfgIdIOBind(5),
  SERIAL_DUMMYIO1,
  SERIAL_DUMMYIO2,
  SERIAL_NOTIFYIO1,
  SERIAL_NOTIFYIO2,
  SERIAL_DUMMYIO3,

  // Path

  CProtoSerial::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Baud rate

  CProtoSerial::cfgIdBaud(),
  115200,
};

// Pack a uint32_t as little-endian into the supplied buffer (4 bytes).

static void serial_pack_le32(uint8_t *buf, uint32_t v)
{
  buf[0] = (uint8_t)(v & 0xFF);
  buf[1] = (uint8_t)((v >> 8) & 0xFF);
  buf[2] = (uint8_t)((v >> 16) & 0xFF);
  buf[3] = (uint8_t)((v >> 24) & 0xFF);
}

// Send a serial protocol frame to the device

static void send_frame(uint8_t cmd, const uint8_t *payload, size_t len)
{
  uint8_t frame[TEST_SIMPLE_PROTO_FRAME_MIN_LEN + TEST_SIMPLE_PROTO_FRAME_MAX_PAYLOAD];
  size_t frame_len;
  ssize_t ret;

  frame_len = test_simple_proto_build_frame(cmd, payload, len, frame);

  ret = write(g_pty_fd, frame, frame_len);
  TEST_ASSERT(ret == (ssize_t)frame_len);

  usleep(100); // Give device time to process
}

// Receive a serial protocol frame from the device

static int recv_frame(uint8_t *cmd, uint8_t *payload, size_t *len)
{
  uint8_t frame[TEST_SIMPLE_PROTO_FRAME_MIN_LEN + TEST_SIMPLE_PROTO_FRAME_MAX_PAYLOAD];
  uint16_t payload_len;
  ssize_t ret;
  size_t frame_len;

  ret = read(g_pty_fd, &frame[0], 3);
  if (ret != 3)
    {
      return -1;
    }

  payload_len = (uint16_t)(frame[1] | (frame[2] << 8));
  frame_len = TEST_SIMPLE_PROTO_FRAME_MIN_LEN + payload_len;
  ret = read(g_pty_fd, &frame[3], frame_len - 3);
  if (ret != (ssize_t)(frame_len - 3))
    {
      return -1;
    }

  return test_simple_proto_parse_frame(frame, frame_len, cmd, payload, len);
}

// Configure + init the four standard IOs, bind the supplied notifier to the
// dummy IOs, configure the serial protocol, and bind the IOs into it.  The
// caller is responsible for serial.init()/start()/stop().

static void serial_setup_simple(CIODummy &d1,
                                CIODummy &d2,
                                CIODummy &d3,
                                CIOSeekableMock &seek,
                                CProtoSerial &serial,
                                CIONotifier &notifier)
{
  TEST_ASSERT_EQUAL(OK, serial.configure());
  TEST_ASSERT_EQUAL(OK, d1.configure());
  TEST_ASSERT_EQUAL(OK, d1.init());
  TEST_ASSERT_EQUAL(OK, d2.configure());
  TEST_ASSERT_EQUAL(OK, d2.init());
  TEST_ASSERT_EQUAL(OK, d3.configure());
  TEST_ASSERT_EQUAL(OK, d3.init());
  TEST_ASSERT_EQUAL(OK, seek.configure());
  TEST_ASSERT_EQUAL(OK, seek.init());

  d1.bindNotifier(&notifier);
  d2.bindNotifier(&notifier);
  d3.bindNotifier(&notifier);

  serial.setObjectMapItem(SERIAL_DUMMYIO1, &d1);
  serial.setObjectMapItem(SERIAL_DUMMYIO2, &d2);
  serial.setObjectMapItem(SERIAL_DUMMYIO3, &d3);
  serial.setObjectMapItem(SERIAL_SEEKIO, &seek);
}

// Counterpart for the notify suite: configure + init three dummy IOs and two
// notify IOs, bind the notifier, and wire up the serial proto.

static void serial_setup_notify(CIODummy &d1,
                                CIODummy &d2,
                                CIODummy &d3,
                                CIODummyNotify &n1,
                                CIODummyNotify &n2,
                                CProtoSerial &serial,
                                CIONotifier &notifier)
{
  TEST_ASSERT_EQUAL(OK, serial.configure());
  TEST_ASSERT_EQUAL(OK, d1.configure());
  TEST_ASSERT_EQUAL(OK, d1.init());
  TEST_ASSERT_EQUAL(OK, d2.configure());
  TEST_ASSERT_EQUAL(OK, d2.init());
  TEST_ASSERT_EQUAL(OK, d3.configure());
  TEST_ASSERT_EQUAL(OK, d3.init());
  TEST_ASSERT_EQUAL(OK, n1.configure());
  TEST_ASSERT_EQUAL(OK, n1.init());
  TEST_ASSERT_EQUAL(OK, n2.configure());
  TEST_ASSERT_EQUAL(OK, n2.init());

  d1.bindNotifier(&notifier);
  d2.bindNotifier(&notifier);
  d3.bindNotifier(&notifier);
  n1.bindNotifier(&notifier);
  n2.bindNotifier(&notifier);

  serial.setObjectMapItem(SERIAL_DUMMYIO1, &d1);
  serial.setObjectMapItem(SERIAL_DUMMYIO2, &d2);
  serial.setObjectMapItem(SERIAL_DUMMYIO3, &d3);
  serial.setObjectMapItem(SERIAL_NOTIFYIO1, &n1);
  serial.setObjectMapItem(SERIAL_NOTIFYIO2, &n2);
}

// Common request/response helper: send a one-objid request, receive a frame,
// and assert it matches the expected command + payload-length.  Returns the
// payload via the caller-provided buffer.

static void serial_send_objid(uint8_t cmd, uint32_t objid)
{
  uint8_t request[4];

  serial_pack_le32(request, objid);
  send_frame(cmd, request, 4);
}

static void serial_recv_expect(uint8_t expected_cmd,
                               size_t expected_len,
                               uint8_t *payload,
                               size_t *payload_len)
{
  uint8_t cmd;
  int ret;

  ret = recv_frame(&cmd, payload, payload_len);
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(expected_cmd, cmd);
  TEST_ASSERT_EQUAL(expected_len, *payload_len);
}

//***************************************************************************
// Description: serial proto runs through start -> hasThread -> stop and
// reports the right hasThread() state on each side.
//***************************************************************************

static void test_proto_serial_lifecycle()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descs(g_cfg_seekio);
  CIOSeekableMock seekio(descs);
  CDescObject desc(g_bin_serial_simple);
  CProtoSerial serial(desc);
  CIONotifier notifier;

  serial_setup_simple(dummy1, dummy2, dummy3, seekio, serial, notifier);

  TEST_ASSERT_EQUAL(false, serial.hasThread());
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());
  TEST_ASSERT_EQUAL(true, serial.hasThread());
  TEST_ASSERT_EQUAL(OK, serial.stop());
  TEST_ASSERT_EQUAL(false, serial.hasThread());
}

//***************************************************************************
// Description: an unrecognised command opcode returns an ERROR frame with
// status STATUS_INVALID.
//***************************************************************************

static void test_proto_serial_invalid_command()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descs(g_cfg_seekio);
  CIOSeekableMock seekio(descs);
  CDescObject desc(g_bin_serial_simple);
  CProtoSerial serial(desc);
  CIONotifier notifier;
  uint8_t payload[256];
  size_t len;

  serial_setup_simple(dummy1, dummy2, dummy3, seekio, serial, notifier);
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());

  send_frame(0xAB, nullptr, 0);
  serial_recv_expect(CMD_ERROR, 2, payload, &len);
  TEST_ASSERT_EQUAL(STATUS_INVALID, payload[0]);

  TEST_ASSERT_EQUAL(OK, serial.stop());
}

//***************************************************************************
// Description: LIST_IOS returns the four bound IO ids.
//***************************************************************************

static void test_proto_serial_list_ios()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descs(g_cfg_seekio);
  CIOSeekableMock seekio(descs);
  CDescObject desc(g_bin_serial_simple);
  CProtoSerial serial(desc);
  CIONotifier notifier;
  uint8_t payload[256];
  size_t len;

  serial_setup_simple(dummy1, dummy2, dummy3, seekio, serial, notifier);
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());

  send_frame(CMD_LIST_IOS, nullptr, 0);

  // Response: [COUNT:2B][OBJID:4B] x COUNT.

  serial_recv_expect(CMD_LIST_IOS, 18, payload, &len);
  TEST_ASSERT_EQUAL(4, payload[0] | (payload[1] << 8));

  TEST_ASSERT_EQUAL(OK, serial.stop());
}

//***************************************************************************
// Description: GET_INFO returns IO_TYPE/DIMENSION/DTYPE for a known IO.
//***************************************************************************

static void test_proto_serial_get_info()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descs(g_cfg_seekio);
  CIOSeekableMock seekio(descs);
  CDescObject desc(g_bin_serial_simple);
  CProtoSerial serial(desc);
  CIONotifier notifier;
  uint8_t payload[256];
  size_t len;

  serial_setup_simple(dummy1, dummy2, dummy3, seekio, serial, notifier);
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());

  serial_send_objid(CMD_GET_INFO, SERIAL_DUMMYIO1);
  serial_recv_expect(CMD_GET_INFO, 3, payload, &len);
  TEST_ASSERT_EQUAL(0x03, payload[0]); // READ-WRITE
  TEST_ASSERT_EQUAL(1, payload[1]);    // DIMENSION
  TEST_ASSERT_EQUAL(SObjectId::DTYPE_INT32, payload[2]);

  TEST_ASSERT_EQUAL(OK, serial.stop());
}

//***************************************************************************
// Description: GET_IO with an unknown object id returns an ERROR frame with
// status STATUS_INVALIDOBJ.
//***************************************************************************

static void test_proto_serial_get_io_invalid_objid()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descs(g_cfg_seekio);
  CIOSeekableMock seekio(descs);
  CDescObject desc(g_bin_serial_simple);
  CProtoSerial serial(desc);
  CIONotifier notifier;
  uint8_t payload[256];
  size_t len;

  serial_setup_simple(dummy1, dummy2, dummy3, seekio, serial, notifier);
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());

  serial_send_objid(CMD_GET_IO, 0xFFFFFFFF);
  serial_recv_expect(CMD_ERROR, 2, payload, &len);
  TEST_ASSERT_EQUAL(STATUS_INVALIDOBJ, payload[0]);

  TEST_ASSERT_EQUAL(OK, serial.stop());
}

//***************************************************************************
// Description: GET_IO returns the dummy's initial value (zero).
//***************************************************************************

static void test_proto_serial_get_io_initial_value()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descs(g_cfg_seekio);
  CIOSeekableMock seekio(descs);
  CDescObject desc(g_bin_serial_simple);
  CProtoSerial serial(desc);
  CIONotifier notifier;
  uint8_t payload[256];
  size_t len;
  uint32_t data;

  serial_setup_simple(dummy1, dummy2, dummy3, seekio, serial, notifier);
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());

  serial_send_objid(CMD_GET_IO, SERIAL_DUMMYIO1);
  serial_recv_expect(CMD_GET_IO, 4, payload, &len);
  std::memcpy(&data, &payload[0], sizeof(data));
  TEST_ASSERT_EQUAL(0, data);

  TEST_ASSERT_EQUAL(OK, serial.stop());
}

//***************************************************************************
// Description: SET_IO with an unknown object id returns an ERROR frame.
//***************************************************************************

static void test_proto_serial_set_io_invalid_objid()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descs(g_cfg_seekio);
  CIOSeekableMock seekio(descs);
  CDescObject desc(g_bin_serial_simple);
  CProtoSerial serial(desc);
  CIONotifier notifier;
  uint8_t request[8];
  uint8_t payload[256];
  size_t len;

  serial_setup_simple(dummy1, dummy2, dummy3, seekio, serial, notifier);
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());

  serial_pack_le32(&request[0], 0xFFFFFFFF);
  serial_pack_le32(&request[4], 0x12345678);
  send_frame(CMD_SET_IO, request, 8);

  serial_recv_expect(CMD_ERROR, 2, payload, &len);
  TEST_ASSERT_EQUAL(STATUS_INVALIDOBJ, payload[0]);

  TEST_ASSERT_EQUAL(OK, serial.stop());
}

//***************************************************************************
// Description: SET_IO followed by GET_IO returns the just-written value.
//***************************************************************************

static void test_proto_serial_set_io_then_get_roundtrip()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descs(g_cfg_seekio);
  CIOSeekableMock seekio(descs);
  CDescObject desc(g_bin_serial_simple);
  CProtoSerial serial(desc);
  CIONotifier notifier;
  uint8_t request[8];
  uint8_t payload[256];
  size_t len;
  uint32_t *data;

  serial_setup_simple(dummy1, dummy2, dummy3, seekio, serial, notifier);
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());

  serial_pack_le32(&request[0], SERIAL_DUMMYIO1);
  serial_pack_le32(&request[4], 0x12345678);
  send_frame(CMD_SET_IO, request, 8);

  serial_recv_expect(CMD_SET_IO, 1, payload, &len);
  TEST_ASSERT_EQUAL(STATUS_OK, payload[0]);

  serial_send_objid(CMD_GET_IO, SERIAL_DUMMYIO1);
  serial_recv_expect(CMD_GET_IO, 4, payload, &len);
  std::memcpy(&data, &payload[0], sizeof(data));
  TEST_ASSERT_EQUAL(0x12345678, data);

  TEST_ASSERT_EQUAL(OK, serial.stop());
}

//***************************************************************************
// Description: SET_IO_SEEK on a non-seekable IO returns an ERROR frame and
// includes the original command opcode in the context byte.
//***************************************************************************

static void test_proto_serial_set_io_seek_non_seekable()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descs(g_cfg_seekio);
  CIOSeekableMock seekio(descs);
  CDescObject desc(g_bin_serial_simple);
  CProtoSerial serial(desc);
  CIONotifier notifier;
  uint8_t request[16];
  uint8_t payload[256];
  size_t len;

  serial_setup_simple(dummy1, dummy2, dummy3, seekio, serial, notifier);
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());

  serial_pack_le32(&request[0], SERIAL_DUMMYIO1);
  serial_pack_le32(&request[4], 0); // offset
  request[8] = 0x5A;
  send_frame(CMD_SET_IO_SEEK, request, 9);

  serial_recv_expect(CMD_ERROR, 2, payload, &len);
  TEST_ASSERT_EQUAL(STATUS_ERROR, payload[0]);
  TEST_ASSERT_EQUAL(CMD_SET_IO_SEEK, payload[1]);

  TEST_ASSERT_EQUAL(OK, serial.stop());
}

//***************************************************************************
// Description: SET_IO_SEEK on a seekable IO succeeds and a subsequent
// GET_IO_SEEK at the same offset returns the bytes that were written.
//***************************************************************************

static void test_proto_serial_seek_roundtrip()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descs(g_cfg_seekio);
  CIOSeekableMock seekio(descs);
  CDescObject desc(g_bin_serial_simple);
  CProtoSerial serial(desc);
  CIONotifier notifier;
  uint8_t request[16];
  uint8_t payload[256];
  size_t len;
  uint8_t cmd;
  int ret;

  serial_setup_simple(dummy1, dummy2, dummy3, seekio, serial, notifier);
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());

  serial_pack_le32(&request[0], SERIAL_SEEKIO);
  serial_pack_le32(&request[4], 4); // offset
  request[8] = 0x11;
  request[9] = 0x22;
  request[10] = 0x33;
  send_frame(CMD_SET_IO_SEEK, request, 11);

  serial_recv_expect(CMD_SET_IO_SEEK, 1, payload, &len);
  TEST_ASSERT_EQUAL(STATUS_OK, payload[0]);

  serial_pack_le32(&request[0], SERIAL_SEEKIO);
  serial_pack_le32(&request[4], 4);
  send_frame(CMD_GET_IO_SEEK, request, 8);

  ret = recv_frame(&cmd, payload, &len);
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(CMD_GET_IO_SEEK, cmd);
  TEST_ASSERT(len >= 11); // hdr(8) + at least 3 bytes chunk
  TEST_ASSERT_EQUAL(0x11, payload[8]);
  TEST_ASSERT_EQUAL(0x22, payload[9]);
  TEST_ASSERT_EQUAL(0x33, payload[10]);

  TEST_ASSERT_EQUAL(OK, serial.stop());
}

//***************************************************************************
// Description: SUBSCRIBE on an unknown object id returns an ERROR frame.
//***************************************************************************

static void test_proto_serial_subscribe_invalid_objid()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descn1(g_cfg_notify1);
  CIODummyNotify notify1(descn1);
  CDescObject descn2(g_cfg_notify2);
  CIODummyNotify notify2(descn2);
  CDescObject desc(g_bin_serial_notify);
  CProtoSerial serial(desc);
  CIONotifier notifier;
  uint8_t payload[256];
  size_t len;

  serial_setup_notify(dummy1, dummy2, dummy3, notify1, notify2, serial, notifier);
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());

  serial_send_objid(CMD_SUBSCRIBE, 0xFFFFFFFF);
  serial_recv_expect(CMD_ERROR, 2, payload, &len);
  TEST_ASSERT_EQUAL(STATUS_INVALIDOBJ, payload[0]);

  TEST_ASSERT_EQUAL(OK, serial.stop());
}

//***************************************************************************
// Description: SUBSCRIBE on a notifiable IO returns STATUS_OK.
//***************************************************************************

static void test_proto_serial_subscribe_valid()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descn1(g_cfg_notify1);
  CIODummyNotify notify1(descn1);
  CDescObject descn2(g_cfg_notify2);
  CIODummyNotify notify2(descn2);
  CDescObject desc(g_bin_serial_notify);
  CProtoSerial serial(desc);
  CIONotifier notifier;
  uint8_t payload[256];
  size_t len;

  serial_setup_notify(dummy1, dummy2, dummy3, notify1, notify2, serial, notifier);
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());

  serial_send_objid(CMD_SUBSCRIBE, SERIAL_NOTIFYIO1);
  serial_recv_expect(CMD_SUBSCRIBE, 1, payload, &len);
  TEST_ASSERT_EQUAL(STATUS_OK, payload[0]);

  TEST_ASSERT_EQUAL(OK, serial.stop());
}

//***************************************************************************
// Description: UNSUBSCRIBE returns STATUS_OK after a prior SUBSCRIBE.
//***************************************************************************

static void test_proto_serial_unsubscribe()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descn1(g_cfg_notify1);
  CIODummyNotify notify1(descn1);
  CDescObject descn2(g_cfg_notify2);
  CIODummyNotify notify2(descn2);
  CDescObject desc(g_bin_serial_notify);
  CProtoSerial serial(desc);
  CIONotifier notifier;
  uint8_t payload[256];
  size_t len;

  serial_setup_notify(dummy1, dummy2, dummy3, notify1, notify2, serial, notifier);
  TEST_ASSERT_EQUAL(OK, serial.init());
  TEST_ASSERT_EQUAL(OK, serial.start());

  serial_send_objid(CMD_SUBSCRIBE, SERIAL_NOTIFYIO1);
  serial_recv_expect(CMD_SUBSCRIBE, 1, payload, &len);
  TEST_ASSERT_EQUAL(STATUS_OK, payload[0]);

  serial_send_objid(CMD_UNSUBSCRIBE, SERIAL_NOTIFYIO1);
  serial_recv_expect(CMD_UNSUBSCRIBE, 1, payload, &len);
  TEST_ASSERT_EQUAL(STATUS_OK, payload[0]);

  TEST_ASSERT_EQUAL(OK, serial.stop());
}

// Wrap a test in open/close PTY since the proto opens its slave end during
// start().  This must be done per test so each test starts with a drained
// master end of the PTY pair.

#define SERIAL_RUN_TEST(fn) \
  do                        \
    {                       \
      open_test_pty();      \
      DAWN_RUN_TEST(fn);    \
      close_test_pty();     \
    }                       \
  while (0)

extern "C"
{
  int test_proto_serial()
  {
    UNITY_BEGIN();

    SERIAL_RUN_TEST(test_proto_serial_lifecycle);
    SERIAL_RUN_TEST(test_proto_serial_invalid_command);
    SERIAL_RUN_TEST(test_proto_serial_list_ios);
    SERIAL_RUN_TEST(test_proto_serial_get_info);
    SERIAL_RUN_TEST(test_proto_serial_get_io_invalid_objid);
    SERIAL_RUN_TEST(test_proto_serial_get_io_initial_value);
    SERIAL_RUN_TEST(test_proto_serial_set_io_invalid_objid);
    SERIAL_RUN_TEST(test_proto_serial_set_io_then_get_roundtrip);
    SERIAL_RUN_TEST(test_proto_serial_set_io_seek_non_seekable);
    SERIAL_RUN_TEST(test_proto_serial_seek_roundtrip);

    SERIAL_RUN_TEST(test_proto_serial_subscribe_invalid_objid);
    SERIAL_RUN_TEST(test_proto_serial_subscribe_valid);
    SERIAL_RUN_TEST(test_proto_serial_unsubscribe);

    return UNITY_END();
  }
}
