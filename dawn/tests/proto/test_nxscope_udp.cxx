// dawn/tests/proto/test_nxscope_udp.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>

#include "dawn/io/dummy.hxx"
#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/fileio.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/io/sysinfo.hxx"
#include "dawn/proto/nxscope/udp.hxx"
#include "logging/nxscope/nxscope.h"
#include "logging/nxscope/nxscope_proto.h"
#include "test_common.hxx"

using namespace dawn;

static constexpr uint16_t NXSCOPE_UDP_PORT_BASIC = 31337;
static constexpr uint16_t NXSCOPE_UDP_PORT_SET = 31338;
static constexpr uint16_t NXSCOPE_UDP_PORT_SEEK = 31339;
static constexpr uint16_t NXSCOPE_UDP_PORT_GET = 31340;
static constexpr char NXSCOPE_TEST_FILE[] = "/tmp/nxscope_udp_set.bin";

static constexpr auto NXSCOPE_NOTIFYIO1 =
  CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto NXSCOPE_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 1);
static constexpr auto NXSCOPE_DUMMYIO16 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 2);
static constexpr auto NXSCOPE_FILEIO1 = CIOFile::objectId(1);
static constexpr auto NXSCOPE_UPTIMEIO = CIOSysinfo::objectIdUptime();

static int g_udp_fd = -1;

static uint32_t g_cfg_notify1[] = {
  NXSCOPE_NOTIFYIO1,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

static uint32_t g_cfg_dummy1[] = {
  NXSCOPE_DUMMYIO1,
  1,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
};

// 64 B dummy - larger than the nxscope lib txbuf (CHINFO sized)

static uint32_t g_cfg_dummy16[] = {
  NXSCOPE_DUMMYIO16,
  1,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_INT32, true, 16),
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16,
};

// Read-only, non-notify IO - binds as a get-only channel

static uint32_t g_cfg_uptime[] = {
  NXSCOPE_UPTIMEIO,
  0,
};

static uint32_t g_cfg_fileio_rw[] = {
  NXSCOPE_FILEIO1,
  2,
  CIOFile::cfgIdPath(6),
  0x706d742f, // /tmp
  0x73786e2f, // /nxs
  0x65706f63, // cope
  0x7064755f, // _udp
  0x7465735f, // _set
  0x6e69622e, // .bin
  CIOFile::cfgIdPerm(),
  CIOFile::IO_FILE_PERM_RW,
};

static uint32_t g_bin_nxscope_udp_basic[] = {
  CProtoNxscopeUdp::objectId(0),
  2,
  CProtoNxscopeUdp::cfgIdPort(),
  NXSCOPE_UDP_PORT_BASIC,
  CProtoNxscopeUdp::cfgIdIOBind2(1),
  NXSCOPE_NOTIFYIO1,
  0x00000061,
  0x00000000,
  0x00000000,
};

static uint32_t g_bin_nxscope_udp_set[] = {
  CProtoNxscopeUdp::objectId(1),
  2,
  CProtoNxscopeUdp::cfgIdPort(),
  NXSCOPE_UDP_PORT_SET,
  CProtoNxscopeUdp::cfgIdIOBind2(2),
  NXSCOPE_DUMMYIO1,
  0x00000061,
  0x00000000,
  0x00000000,
  NXSCOPE_FILEIO1,
  0x00000066,
  0x00000000,
  0x00000000,
};

static uint32_t g_bin_nxscope_udp_seek[] = {
  CProtoNxscopeUdp::objectId(2),
  2,
  CProtoNxscopeUdp::cfgIdPort(),
  NXSCOPE_UDP_PORT_SEEK,
  CProtoNxscopeUdp::cfgIdIOBind2(2),
  NXSCOPE_DUMMYIO1,
  0x00000061,
  0x00000000,
  0x00000000,
  NXSCOPE_FILEIO1,
  0x00000066,
  0x00000000,
  0x00000000,
};

static uint32_t g_bin_nxscope_udp_get[] = {
  CProtoNxscopeUdp::objectId(3),
  2,
  CProtoNxscopeUdp::cfgIdPort(),
  NXSCOPE_UDP_PORT_GET,
  CProtoNxscopeUdp::cfgIdIOBind2(4),
  NXSCOPE_DUMMYIO1,
  0x00000061,
  0x00000000,
  0x00000000,
  NXSCOPE_FILEIO1,
  0x00000066,
  0x00000000,
  0x00000000,
  NXSCOPE_UPTIMEIO,
  0x00000075,
  0x00000000,
  0x00000000,
  NXSCOPE_DUMMYIO16,
  0x00000062,
  0x00000000,
  0x00000000,
};

static int send_user_frame_udp(uint16_t port, uint8_t id, const uint8_t *payload, size_t n)
{
  struct nxscope_proto_s proto;
  struct sockaddr_in addr;
  uint8_t frame[128];
  size_t len;
  ssize_t ret;

  TEST_ASSERT(payload != nullptr);
  TEST_ASSERT(n < sizeof(frame));
  TEST_ASSERT(g_udp_fd >= 0);

  ret = nxscope_proto_ser_init(&proto, nullptr);
  TEST_ASSERT_EQUAL(OK, ret);

  len = proto.hdrlen;
  std::memcpy(&frame[len], payload, n);
  len += n;

  ret = proto.ops->frame_final(&proto, id, frame, &len);
  TEST_ASSERT_EQUAL(OK, ret);

  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  ret = sendto(g_udp_fd, frame, len, 0, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
  TEST_ASSERT_EQUAL((ssize_t)len, ret);

  nxscope_proto_ser_deinit(&proto);
  return OK;
}

// Wait for a datagram carrying a frame with the given id, skipping others
// (ACKs). Returns the frame payload.

static int recv_user_frame_udp(uint8_t id, uint8_t *out, size_t *outlen)
{
  struct nxscope_proto_s proto;
  struct nxscope_frame_s frame;
  struct pollfd pfd;
  uint8_t buf[256];
  ssize_t n;
  int i;

  TEST_ASSERT_EQUAL(OK, nxscope_proto_ser_init(&proto, nullptr));

  for (i = 0; i < 50; i++)
    {
      pfd.fd = g_udp_fd;
      pfd.events = POLLIN;
      pfd.revents = 0;
      if (poll(&pfd, 1, 20) <= 0)
        {
          continue;
        }

      n = recvfrom(g_udp_fd, buf, sizeof(buf), 0, nullptr, nullptr);
      if (n > (ssize_t)(proto.hdrlen + proto.footlen) &&
          proto.ops->frame_get(&proto, buf, n, &frame) == OK && frame.id == id)
        {
          std::memcpy(out, frame.data, frame.dlen);
          *outlen = frame.dlen;
          nxscope_proto_ser_deinit(&proto);
          return OK;
        }
    }

  nxscope_proto_ser_deinit(&proto);
  return -ETIMEDOUT;
}

// Configure + init + bind all four IOs, configure + start nxscope on the
// GET port. Caller stops nxscope.

static void nxscope_setup_get(CIODummy &dummy1,
                              CIOFile &fileio,
                              CIOSysinfo &uptime,
                              CIODummy &dummy16,
                              CProtoNxscopeUdp &nxscope)
{
  int fd;

  mkdir("/tmp", 0777);
  mount(nullptr, "/tmp", "tmpfs", 0, nullptr);

  fd = open(NXSCOPE_TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(fd >= 0);
  TEST_ASSERT(write(fd, "00000000", 8) == 8);
  close(fd);

  TEST_ASSERT_EQUAL(OK, nxscope.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  TEST_ASSERT_EQUAL(OK, fileio.configure());
  TEST_ASSERT_EQUAL(OK, fileio.init());
  TEST_ASSERT_EQUAL(OK, uptime.configure());
  TEST_ASSERT_EQUAL(OK, uptime.init());
  TEST_ASSERT_EQUAL(OK, dummy16.configure());
  TEST_ASSERT_EQUAL(OK, dummy16.init());
  nxscope.setObjectMapItem(NXSCOPE_DUMMYIO1, &dummy1);
  nxscope.setObjectMapItem(NXSCOPE_FILEIO1, &fileio);
  nxscope.setObjectMapItem(NXSCOPE_UPTIMEIO, &uptime);
  nxscope.setObjectMapItem(NXSCOPE_DUMMYIO16, &dummy16);
  TEST_ASSERT_EQUAL(OK, nxscope.init());
  TEST_ASSERT_EQUAL(OK, nxscope.start());
}

static void test_proto_nxscope_udp_basic()
{
  CDescObject descv1(g_cfg_notify1);
  CIODummyNotify notify1(descv1);
  CDescObject desc(g_bin_nxscope_udp_basic);
  CProtoNxscopeUdp nxscope(desc);
  CIONotifier notifier;
  int ret;

  TEST_ASSERT_EQUAL(OK, nxscope.configure());
  TEST_ASSERT_EQUAL(OK, notify1.configure());
  TEST_ASSERT_EQUAL(OK, notify1.init());

  notify1.bindNotifier(&notifier);
  nxscope.setObjectMapItem(NXSCOPE_NOTIFYIO1, &notify1);

  TEST_ASSERT_EQUAL(false, nxscope.hasThread());
  TEST_ASSERT_EQUAL(OK, nxscope.init());

  ret = nxscope.start();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_EQUAL(true, nxscope.hasThread());

  ret = nxscope.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = nxscope.deinit();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = notifier.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(false, nxscope.hasThread());
}

static void test_proto_nxscope_udp_user_set_io_simple()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject desc(g_bin_nxscope_udp_set);
  CProtoNxscopeUdp nxscope(desc);
  io_ddata_t *chunk;
  uint8_t payload[13];
  uint32_t objid;
  uint16_t size;
  int32_t *out;
  int ret;
  int i;
  bool updated;

  mkdir("/tmp", 0777);
  mount(nullptr, "/tmp", "tmpfs", 0, nullptr);

  ret = open(NXSCOPE_TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(ret >= 0);
  close(ret);

  TEST_ASSERT_EQUAL(OK, nxscope.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  TEST_ASSERT_EQUAL(OK, fileio.configure());
  TEST_ASSERT_EQUAL(OK, fileio.init());
  nxscope.setObjectMapItem(NXSCOPE_DUMMYIO1, &dummy1);
  nxscope.setObjectMapItem(NXSCOPE_FILEIO1, &fileio);
  TEST_ASSERT_EQUAL(OK, nxscope.init());
  TEST_ASSERT_EQUAL(OK, nxscope.start());

  objid = NXSCOPE_DUMMYIO1;
  size = sizeof(int32_t);
  std::memcpy(&payload[0], &objid, sizeof(objid));
  std::memcpy(&payload[4], &size, sizeof(size));
  payload[6] = 0x78;
  payload[7] = 0x56;
  payload[8] = 0x34;
  payload[9] = 0x12;

  TEST_ASSERT_EQUAL(OK, send_user_frame_udp(NXSCOPE_UDP_PORT_SET, NXSCOPE_HDRID_USER, payload, 10));

  updated = false;
  for (i = 0; i < 3; i++)
    {
      usleep(60000);
      chunk = dummy1.ddata_alloc(1);
      TEST_ASSERT(chunk != nullptr);
      TEST_ASSERT_EQUAL(OK, dummy1.getData(*chunk, 1));
      out = static_cast<int32_t *>(chunk->getDataPtr(0));
      if (*out == 0x12345678)
        {
          updated = true;
          free(chunk);
          break;
        }

      free(chunk);
    }

  TEST_ASSERT_EQUAL(true, updated);
  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  TEST_ASSERT_EQUAL(OK, nxscope.deinit());
  dummy1.deinit();
  fileio.deinit();
  unlink(NXSCOPE_TEST_FILE);
}

static void test_proto_nxscope_udp_user_set_io_seek()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject desc(g_bin_nxscope_udp_seek);
  CProtoNxscopeUdp nxscope(desc);
  io_ddata_t *chunk;
  uint8_t *ptr;
  uint8_t set_payload[13];
  uint8_t seek_payload[14];
  uint32_t objid;
  uint32_t offset;
  uint16_t size;
  int fd;
  int i;
  bool seek_updated;

  mkdir("/tmp", 0777);
  mount(nullptr, "/tmp", "tmpfs", 0, nullptr);

  fd = open(NXSCOPE_TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(fd >= 0);
  TEST_ASSERT(write(fd, "00000000", 8) == 8);
  close(fd);

  TEST_ASSERT_EQUAL(OK, nxscope.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  TEST_ASSERT_EQUAL(OK, fileio.configure());
  TEST_ASSERT_EQUAL(OK, fileio.init());
  nxscope.setObjectMapItem(NXSCOPE_DUMMYIO1, &dummy1);
  nxscope.setObjectMapItem(NXSCOPE_FILEIO1, &fileio);
  TEST_ASSERT_EQUAL(OK, nxscope.init());
  TEST_ASSERT_EQUAL(OK, nxscope.start());

  objid = NXSCOPE_FILEIO1;
  size = 3;
  std::memcpy(&set_payload[0], &objid, sizeof(objid));
  std::memcpy(&set_payload[4], &size, sizeof(size));
  std::memcpy(&set_payload[6], "ABC", 3);
  TEST_ASSERT_EQUAL(OK,
                    send_user_frame_udp(
                      NXSCOPE_UDP_PORT_SEEK, NXSCOPE_HDRID_USER, set_payload, sizeof(set_payload)));

  objid = NXSCOPE_FILEIO1;
  offset = 2;
  size = 4;
  std::memcpy(&seek_payload[0], &objid, sizeof(objid));
  std::memcpy(&seek_payload[4], &offset, sizeof(offset));
  std::memcpy(&seek_payload[8], &size, sizeof(size));
  std::memcpy(&seek_payload[10], "WXYZ", 4);
  seek_updated = false;
  for (i = 0; i < 3; i++)
    {
      TEST_ASSERT_EQUAL(
        OK,
        send_user_frame_udp(
          NXSCOPE_UDP_PORT_SEEK, NXSCOPE_HDRID_USER + 1, seek_payload, sizeof(seek_payload)));
      usleep(60000);

      chunk = fileio.ddata_alloc(1, 8);
      TEST_ASSERT(chunk != nullptr);
      TEST_ASSERT_EQUAL(OK, fileio.getData(*chunk, 1, 0));
      ptr = static_cast<uint8_t *>(chunk->getDataPtr(0));
      if (ptr[0] == 'A' && ptr[1] == 'B' && ptr[2] == 'W' && ptr[3] == 'X' && ptr[4] == 'Y' &&
          ptr[5] == 'Z')
        {
          seek_updated = true;
          free(chunk);
          break;
        }

      free(chunk);
    }

  TEST_ASSERT_EQUAL(true, seek_updated);
  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  TEST_ASSERT_EQUAL(OK, nxscope.deinit());
  dummy1.deinit();
  fileio.deinit();
  unlink(NXSCOPE_TEST_FILE);
}

//***************************************************************************
// Description: GET_IO (USER+2) returns the value written with SET_IO as
// [objid:4][size:2][data]; a read-only non-notify IO (uptime) binds as a
// get-only channel instead of failing init and answers GET_IO too.
//***************************************************************************

static void test_proto_nxscope_udp_user_get_io()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject descv3(g_cfg_uptime);
  CIOSysinfo uptime(descv3);
  CDescObject descv4(g_cfg_dummy16);
  CIODummy dummy16(descv4);
  CDescObject desc(g_bin_nxscope_udp_get);
  CProtoNxscopeUdp nxscope(desc);
  uint8_t payload[10];
  uint8_t resp[128];
  size_t n;
  uint32_t objid;
  uint16_t size;
  int32_t value;

  nxscope_setup_get(dummy1, fileio, uptime, dummy16, nxscope);

  objid = NXSCOPE_DUMMYIO1;
  size = sizeof(int32_t);
  value = 0x12345678;
  std::memcpy(&payload[0], &objid, sizeof(objid));
  std::memcpy(&payload[4], &size, sizeof(size));
  std::memcpy(&payload[6], &value, sizeof(value));
  TEST_ASSERT_EQUAL(
    OK, send_user_frame_udp(NXSCOPE_UDP_PORT_GET, NXSCOPE_HDRID_USER, payload, sizeof(payload)));

  // Back-to-back datagrams would overflow the device RX buffer

  usleep(60000);
  TEST_ASSERT_EQUAL(
    OK, send_user_frame_udp(NXSCOPE_UDP_PORT_GET, NXSCOPE_HDRID_USER + 2, payload, sizeof(objid)));
  TEST_ASSERT_EQUAL(OK, recv_user_frame_udp(NXSCOPE_HDRID_USER + 2, resp, &n));
  TEST_ASSERT_EQUAL(6 + sizeof(int32_t), n);
  std::memcpy(&objid, &resp[0], sizeof(objid));
  std::memcpy(&size, &resp[4], sizeof(size));
  std::memcpy(&value, &resp[6], sizeof(value));
  TEST_ASSERT_EQUAL(NXSCOPE_DUMMYIO1, objid);
  TEST_ASSERT_EQUAL(sizeof(int32_t), size);
  TEST_ASSERT_EQUAL(0x12345678, value);

  objid = NXSCOPE_UPTIMEIO;
  std::memcpy(&payload[0], &objid, sizeof(objid));
  TEST_ASSERT_EQUAL(
    OK, send_user_frame_udp(NXSCOPE_UDP_PORT_GET, NXSCOPE_HDRID_USER + 2, payload, sizeof(objid)));
  TEST_ASSERT_EQUAL(OK, recv_user_frame_udp(NXSCOPE_HDRID_USER + 2, resp, &n));
  TEST_ASSERT_EQUAL(6 + sizeof(uint64_t), n);
  std::memcpy(&objid, &resp[0], sizeof(objid));
  std::memcpy(&size, &resp[4], sizeof(size));
  TEST_ASSERT_EQUAL(NXSCOPE_UPTIMEIO, objid);
  TEST_ASSERT_EQUAL(sizeof(uint64_t), size);

  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  TEST_ASSERT_EQUAL(OK, nxscope.deinit());
  dummy1.deinit();
  dummy16.deinit();
  uptime.deinit();
  fileio.deinit();
  unlink(NXSCOPE_TEST_FILE);
}

//***************************************************************************
// Description: GET_IO_SEEK (USER+3) reads a window of the seekable fileio
// after SET_IO_SEEK wrote "WXYZ" at offset 2 (file seeded with "00000000").
//***************************************************************************

static void test_proto_nxscope_udp_user_get_io_seek()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject descv3(g_cfg_uptime);
  CIOSysinfo uptime(descv3);
  CDescObject descv4(g_cfg_dummy16);
  CIODummy dummy16(descv4);
  CDescObject desc(g_bin_nxscope_udp_get);
  CProtoNxscopeUdp nxscope(desc);
  uint8_t set_payload[14];
  uint8_t seek_payload[10];
  uint8_t resp[128];
  size_t n;
  uint32_t objid;
  uint32_t offset;
  uint16_t size;

  nxscope_setup_get(dummy1, fileio, uptime, dummy16, nxscope);

  objid = NXSCOPE_FILEIO1;
  offset = 2;
  size = 4;
  std::memcpy(&set_payload[0], &objid, sizeof(objid));
  std::memcpy(&set_payload[4], &offset, sizeof(offset));
  std::memcpy(&set_payload[8], &size, sizeof(size));
  std::memcpy(&set_payload[10], "WXYZ", 4);
  TEST_ASSERT_EQUAL(
    OK,
    send_user_frame_udp(
      NXSCOPE_UDP_PORT_GET, NXSCOPE_HDRID_USER + 1, set_payload, sizeof(set_payload)));

  usleep(60000);
  offset = 1;
  size = 4;
  std::memcpy(&seek_payload[0], &objid, sizeof(objid));
  std::memcpy(&seek_payload[4], &offset, sizeof(offset));
  std::memcpy(&seek_payload[8], &size, sizeof(size));
  TEST_ASSERT_EQUAL(
    OK,
    send_user_frame_udp(
      NXSCOPE_UDP_PORT_GET, NXSCOPE_HDRID_USER + 3, seek_payload, sizeof(seek_payload)));
  TEST_ASSERT_EQUAL(OK, recv_user_frame_udp(NXSCOPE_HDRID_USER + 2, resp, &n));
  TEST_ASSERT_EQUAL(6 + 4, n);
  std::memcpy(&objid, &resp[0], sizeof(objid));
  std::memcpy(&size, &resp[4], sizeof(size));
  TEST_ASSERT_EQUAL(NXSCOPE_FILEIO1, objid);
  TEST_ASSERT_EQUAL(4, size);
  TEST_ASSERT_EQUAL(0, std::memcmp(&resp[6], "0WXY", 4));

  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  TEST_ASSERT_EQUAL(OK, nxscope.deinit());
  dummy1.deinit();
  dummy16.deinit();
  uptime.deinit();
  fileio.deinit();
  unlink(NXSCOPE_TEST_FILE);
}

//***************************************************************************
// Description: GET_IO of a 64 B IO fits the proto's own response buffer
// (the nxscope lib txbuf is CHINFO sized and would return -ENOBUFS).
//***************************************************************************

static void test_proto_nxscope_udp_user_get_io_large()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject descv3(g_cfg_uptime);
  CIOSysinfo uptime(descv3);
  CDescObject descv4(g_cfg_dummy16);
  CIODummy dummy16(descv4);
  CDescObject desc(g_bin_nxscope_udp_get);
  CProtoNxscopeUdp nxscope(desc);
  uint8_t resp[128];
  size_t n;
  uint32_t objid;
  uint16_t size;
  int32_t value;
  int i;

  nxscope_setup_get(dummy1, fileio, uptime, dummy16, nxscope);

  objid = NXSCOPE_DUMMYIO16;
  TEST_ASSERT_EQUAL(
    OK,
    send_user_frame_udp(
      NXSCOPE_UDP_PORT_GET, NXSCOPE_HDRID_USER + 2, (uint8_t *)&objid, sizeof(objid)));
  TEST_ASSERT_EQUAL(OK, recv_user_frame_udp(NXSCOPE_HDRID_USER + 2, resp, &n));
  TEST_ASSERT_EQUAL(6 + 64, n);
  std::memcpy(&objid, &resp[0], sizeof(objid));
  std::memcpy(&size, &resp[4], sizeof(size));
  TEST_ASSERT_EQUAL(NXSCOPE_DUMMYIO16, objid);
  TEST_ASSERT_EQUAL(64, size);
  for (i = 0; i < 16; i++)
    {
      std::memcpy(&value, &resp[6 + 4 * i], sizeof(value));
      TEST_ASSERT_EQUAL(i + 1, value);
    }

  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  TEST_ASSERT_EQUAL(OK, nxscope.deinit());
  dummy1.deinit();
  dummy16.deinit();
  uptime.deinit();
  fileio.deinit();
  unlink(NXSCOPE_TEST_FILE);
}

extern "C"
{
  int test_proto_nxscope_udp()
  {
    g_udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    TEST_ASSERT(g_udp_fd > 0);

    UNITY_BEGIN();
    DAWN_RUN_TEST(test_proto_nxscope_udp_basic);
    DAWN_RUN_TEST(test_proto_nxscope_udp_user_set_io_simple);
    DAWN_RUN_TEST(test_proto_nxscope_udp_user_set_io_seek);
    DAWN_RUN_TEST(test_proto_nxscope_udp_user_get_io);
    DAWN_RUN_TEST(test_proto_nxscope_udp_user_get_io_seek);
    DAWN_RUN_TEST(test_proto_nxscope_udp_user_get_io_large);
    close(g_udp_fd);
    return UNITY_END();
  }
}
