// dawn/tests/proto/test_nxscope_serial.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "dawn/io/dummy.hxx"
#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/fileio.hxx"
#include "dawn/io/sysinfo.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/proto/nxscope/serial.hxx"
#include "logging/nxscope/nxscope.h"
#include "logging/nxscope/nxscope_proto.h"
#include "test_common.hxx"

using namespace dawn;

#ifndef CONFIG_DAWN_IO_FILE
#  error CONFIG_DAWN_IO_FILE must be enabled
#endif

#ifndef CONFIG_DAWN_IO_SYSINFO
#  error CONFIG_DAWN_IO_SYSINFO must be enabled
#endif

static constexpr auto NXSCOPE_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 1);
static constexpr auto NXSCOPE_DUMMYIO16 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 2);
static constexpr auto NXSCOPE_FILEIO1 = CIOFile::objectId(1);
static constexpr auto NXSCOPE_UPTIMEIO = CIOSysinfo::objectIdUptime();
static constexpr auto NXSCOPE_VIRTIO1 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 3);
static constexpr auto NXSCOPE_NOTIFYIO1 =
  CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 4);
static constexpr auto NXSCOPE_TEST_FILE = "/tmp/nxset.bin";

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

// Notify-capable virt - binds as a stream channel

static uint32_t g_cfg_virt1[] = {
  NXSCOPE_VIRTIO1,
  0,
};

// Notify-capable IO left without a bound notifier - setNotifier() fails

static uint32_t g_cfg_notify1[] = {
  NXSCOPE_NOTIFYIO1,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

static uint32_t g_cfg_fileio_rw[] = {
  NXSCOPE_FILEIO1,
  2,
  CIOFile::cfgIdPath(4),
  0x706d742f, // /tmp
  0x73786e2f, // /nxs
  0x622e7465, // et.b
  0x00006e69, // in\0
  CIOFile::cfgIdPerm(),
  CIOFile::IO_FILE_PERM_RW,
};

static uint32_t g_bin_nxscope_serial_seek[] = {
  CProtoNxscopeSerial::objectId(1),
  2,
  CProtoNxscopeSerial::cfgIdPath(3),
  0x7665642f, // /dev
  0x7974742f, // /tty
  0x00003070, // p0
  CProtoNxscopeSerial::cfgIdIOBind2(2),
  NXSCOPE_DUMMYIO1,
  0x00000061,
  0x00000000,
  0x00000000,
  NXSCOPE_FILEIO1,
  0x00000066,
  0x00000000,
  0x00000000,
};

static uint32_t g_bin_nxscope_serial_get[] = {
  CProtoNxscopeSerial::objectId(2),
  2,
  CProtoNxscopeSerial::cfgIdPath(3),
  0x7665642f, // /dev
  0x7974742f, // /tty
  0x00003070, // p0
  CProtoNxscopeSerial::cfgIdIOBind2(4),
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

static uint32_t g_bin_nxscope_serial_virt[] = {
  CProtoNxscopeSerial::objectId(3),
  2,
  CProtoNxscopeSerial::cfgIdPath(3),
  0x7665642f, // /dev
  0x7974742f, // /tty
  0x00003070, // p0
  CProtoNxscopeSerial::cfgIdIOBind2(3),
  NXSCOPE_DUMMYIO1,
  0x00000061,
  0x00000000,
  0x00000000,
  NXSCOPE_FILEIO1,
  0x00000066,
  0x00000000,
  0x00000000,
  NXSCOPE_VIRTIO1,
  0x00000076,
  0x00000000,
  0x00000000,
};

static uint32_t g_bin_nxscope_serial_notify[] = {
  CProtoNxscopeSerial::objectId(4),
  2,
  CProtoNxscopeSerial::cfgIdPath(3),
  0x7665642f, // /dev
  0x7974742f, // /tty
  0x00003070, // p0
  CProtoNxscopeSerial::cfgIdIOBind2(3),
  NXSCOPE_DUMMYIO1,
  0x00000061,
  0x00000000,
  0x00000000,
  NXSCOPE_FILEIO1,
  0x00000066,
  0x00000000,
  0x00000000,
  NXSCOPE_NOTIFYIO1,
  0x0000006e,
  0x00000000,
  0x00000000,
};

static int open_test_pty()
{
  int fd;
  int ret;
  struct termios tio;

  fd = open("/dev/pty0", O_RDWR);
  TEST_ASSERT(fd > 0);

  ret = unlockpt(fd);
  TEST_ASSERT_EQUAL(0, ret);

  tcgetattr(fd, &tio);
  cfmakeraw(&tio);
  tcsetattr(fd, TCSANOW, &tio);
  tcflush(fd, TCIOFLUSH);

  dawn_test_drain_pty_master(fd);

  return fd;
}

static int open_test_ttyp_nonblock()
{
  int fd;
  int flags;
  int ret;

  fd = open("/dev/ttyp0", O_RDWR);
  TEST_ASSERT(fd > 0);

  flags = fcntl(fd, F_GETFL, 0);
  TEST_ASSERT(flags >= 0);

  ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  TEST_ASSERT_EQUAL(0, ret);

  return fd;
}

static int send_user_frame(int fd, uint8_t id, const uint8_t *payload, size_t n)
{
  struct nxscope_proto_s proto;
  uint8_t frame[128];
  size_t len;
  int ret;

  TEST_ASSERT(payload != nullptr);
  TEST_ASSERT(n < sizeof(frame));

  ret = nxscope_proto_ser_init(&proto, nullptr);
  TEST_ASSERT_EQUAL(OK, ret);

  len = proto.hdrlen;
  std::memcpy(&frame[len], payload, n);
  len += n;

  ret = proto.ops->frame_final(&proto, id, frame, &len);
  TEST_ASSERT_EQUAL(OK, ret);

  ret = write(fd, frame, len);
  TEST_ASSERT_EQUAL((int)len, ret);

  nxscope_proto_ser_deinit(&proto);
  return OK;
}

// Wait for a frame with the given id on the PTY master, skipping others
// (ACKs). Returns the frame payload.

static int recv_user_frame(int fd, uint8_t id, uint8_t *out, size_t *outlen)
{
  struct nxscope_proto_s proto;
  struct nxscope_frame_s frame;
  struct pollfd pfd;
  uint8_t buf[256];
  size_t len = 0;
  ssize_t n;
  int i;

  TEST_ASSERT_EQUAL(OK, nxscope_proto_ser_init(&proto, nullptr));

  for (i = 0; i < 50; i++)
    {
      pfd.fd = fd;
      pfd.events = POLLIN;
      pfd.revents = 0;
      if (poll(&pfd, 1, 20) > 0)
        {
          n = read(fd, &buf[len], sizeof(buf) - len);
          if (n > 0)
            {
              len += n;
            }
        }

      while (len > proto.hdrlen + proto.footlen &&
             proto.ops->frame_get(&proto, buf, len, &frame) == OK)
        {
          if (frame.id == id)
            {
              std::memcpy(out, frame.data, frame.dlen);
              *outlen = frame.dlen;
              nxscope_proto_ser_deinit(&proto);
              return OK;
            }

          std::memmove(buf, &buf[frame.drop], len - frame.drop);
          len -= frame.drop;
        }
    }

  nxscope_proto_ser_deinit(&proto);
  return -ETIMEDOUT;
}

// Configure + init + bind dummy + fileio (+ optional uptime, dummy16 and one
// extra IO), configure + start nxscope. Caller closes the PTY fds and stops
// nxscope.

static void nxscope_setup(CIODummy &dummy,
                          CIOFile &fileio,
                          CProtoNxscopeSerial &nxscope,
                          int *out_pty_fd,
                          int *out_ttyp_fd,
                          CIOSysinfo *uptime = nullptr,
                          CIODummy *dummy16 = nullptr,
                          CIOCommon *extra = nullptr,
                          SObjectId::ObjectId extraId = 0,
                          int startRet = OK)
{
  mkdir("/tmp", 0777);
  mount(nullptr, "/tmp", "tmpfs", 0, nullptr);

  int seed_fd = open(NXSCOPE_TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(seed_fd >= 0);
  TEST_ASSERT(write(seed_fd, "00000000", 8) == 8);
  close(seed_fd);

  *out_pty_fd = open_test_pty();
  *out_ttyp_fd = open_test_ttyp_nonblock();

  TEST_ASSERT_EQUAL(OK, nxscope.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  TEST_ASSERT_EQUAL(OK, fileio.configure());
  TEST_ASSERT_EQUAL(OK, fileio.init());
  nxscope.setObjectMapItem(NXSCOPE_DUMMYIO1, &dummy);
  nxscope.setObjectMapItem(NXSCOPE_FILEIO1, &fileio);
  if (uptime != nullptr)
    {
      TEST_ASSERT_EQUAL(OK, uptime->configure());
      TEST_ASSERT_EQUAL(OK, uptime->init());
      nxscope.setObjectMapItem(NXSCOPE_UPTIMEIO, uptime);
    }
  if (dummy16 != nullptr)
    {
      TEST_ASSERT_EQUAL(OK, dummy16->configure());
      TEST_ASSERT_EQUAL(OK, dummy16->init());
      nxscope.setObjectMapItem(NXSCOPE_DUMMYIO16, dummy16);
    }
  if (extra != nullptr)
    {
      TEST_ASSERT_EQUAL(OK, extra->configure());
      TEST_ASSERT_EQUAL(OK, extra->init());
      nxscope.setObjectMapItem(extraId, extra);
    }
  TEST_ASSERT_EQUAL(OK, nxscope.init());
  TEST_ASSERT_EQUAL(startRet, nxscope.start());
}

// Wait for the next ACK frame and return its status

static int recv_ack(int fd)
{
  uint8_t resp[16];
  size_t n = 0;
  int ret;

  TEST_ASSERT_EQUAL(OK, recv_user_frame(fd, NXSCOPE_HDRID_ACK, resp, &n));
  TEST_ASSERT_EQUAL(sizeof(ret), n);
  std::memcpy(&ret, resp, sizeof(ret));
  return ret;
}

//***************************************************************************
// Description: a USER frame carrying a SET_IO command writes the supplied
// payload to the bound fileio at offset 0.
//***************************************************************************

static void test_proto_nxscope_serial_user_set_io_simple()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject desc(g_bin_nxscope_serial_seek);
  CProtoNxscopeSerial nxscope(desc);
  uint8_t set_payload[13];
  uint32_t objid;
  uint16_t size;
  int fd;
  int ttypfd;
  io_ddata_t *chunk;
  uint8_t *ptr;
  int i;
  bool seen;

  nxscope_setup(dummy1, fileio, nxscope, &fd, &ttypfd);

  objid = NXSCOPE_FILEIO1;
  size = 3;
  std::memcpy(&set_payload[0], &objid, sizeof(objid));
  std::memcpy(&set_payload[4], &size, sizeof(size));
  std::memcpy(&set_payload[6], "ABC", 3);
  TEST_ASSERT_EQUAL(OK, send_user_frame(fd, NXSCOPE_HDRID_USER, set_payload, sizeof(set_payload)));
  dawn_test_drain_pty_master(fd);

  seen = false;
  for (i = 0; i < 3; i++)
    {
      usleep(60000);
      chunk = fileio.ddata_alloc(1, 8);
      TEST_ASSERT(chunk != nullptr);
      TEST_ASSERT_EQUAL(OK, fileio.getData(*chunk, 1, 0));
      ptr = static_cast<uint8_t *>(chunk->getDataPtr(0));
      if (ptr[0] == 'A' && ptr[1] == 'B' && ptr[2] == 'C')
        {
          seen = true;
          free(chunk);
          break;
        }
      free(chunk);
    }
  TEST_ASSERT_EQUAL(true, seen);

  dawn_test_drain_pty_master(fd);
  close(fd);
  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  close(ttypfd);
  dummy1.deinit();
  fileio.deinit();
  unlink(NXSCOPE_TEST_FILE);
}

//***************************************************************************
// Description: a USER frame carrying SET_IO followed by a USER+1 frame
// carrying SET_IO_SEEK writes payload at offset 2; the resulting file
// contains the merged "ABWXYZ" pattern.
//***************************************************************************

static void test_proto_nxscope_serial_user_set_io_seek()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject desc(g_bin_nxscope_serial_seek);
  CProtoNxscopeSerial nxscope(desc);
  uint8_t set_payload[13];
  uint8_t seek_payload[14];
  uint32_t objid;
  uint32_t offset;
  uint16_t size;
  io_ddata_t *chunk;
  uint8_t *ptr;
  int i;
  bool seek_updated;
  int fd;
  int ttypfd;

  nxscope_setup(dummy1, fileio, nxscope, &fd, &ttypfd);

  objid = NXSCOPE_FILEIO1;
  size = 3;
  std::memcpy(&set_payload[0], &objid, sizeof(objid));
  std::memcpy(&set_payload[4], &size, sizeof(size));
  std::memcpy(&set_payload[6], "ABC", 3);
  TEST_ASSERT_EQUAL(OK, send_user_frame(fd, NXSCOPE_HDRID_USER, set_payload, sizeof(set_payload)));
  dawn_test_drain_pty_master(fd);

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
        OK, send_user_frame(fd, NXSCOPE_HDRID_USER + 1, seek_payload, sizeof(seek_payload)));
      dawn_test_drain_pty_master(fd);
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

  dawn_test_drain_pty_master(fd);
  close(fd);
  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  close(ttypfd);
  dummy1.deinit();
  fileio.deinit();
  unlink(NXSCOPE_TEST_FILE);
}

//***************************************************************************
// Description: GET_IO (USER+2) returns the value written with SET_IO as
// [objid:4][size:2][data]; a read-only non-notify IO (uptime) binds as a
// get-only channel instead of failing init and answers GET_IO too.
//***************************************************************************

static void test_proto_nxscope_serial_user_get_io()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject descv3(g_cfg_uptime);
  CIOSysinfo uptime(descv3);
  CDescObject descv4(g_cfg_dummy16);
  CIODummy dummy16(descv4);
  CDescObject desc(g_bin_nxscope_serial_get);
  CProtoNxscopeSerial nxscope(desc);
  uint8_t payload[10];
  uint8_t resp[128];
  size_t n;
  uint32_t objid;
  uint16_t size;
  int32_t value;
  int fd;
  int ttypfd;

  nxscope_setup(dummy1, fileio, nxscope, &fd, &ttypfd, &uptime, &dummy16);

  objid = NXSCOPE_DUMMYIO1;
  size = sizeof(int32_t);
  value = 0x12345678;
  std::memcpy(&payload[0], &objid, sizeof(objid));
  std::memcpy(&payload[4], &size, sizeof(size));
  std::memcpy(&payload[6], &value, sizeof(value));
  TEST_ASSERT_EQUAL(OK, send_user_frame(fd, NXSCOPE_HDRID_USER, payload, sizeof(payload)));

  TEST_ASSERT_EQUAL(OK, send_user_frame(fd, NXSCOPE_HDRID_USER + 2, payload, sizeof(objid)));
  TEST_ASSERT_EQUAL(OK, recv_user_frame(fd, NXSCOPE_HDRID_USER + 2, resp, &n));
  TEST_ASSERT_EQUAL(6 + sizeof(int32_t), n);
  std::memcpy(&objid, &resp[0], sizeof(objid));
  std::memcpy(&size, &resp[4], sizeof(size));
  std::memcpy(&value, &resp[6], sizeof(value));
  TEST_ASSERT_EQUAL(NXSCOPE_DUMMYIO1, objid);
  TEST_ASSERT_EQUAL(sizeof(int32_t), size);
  TEST_ASSERT_EQUAL(0x12345678, value);

  objid = NXSCOPE_UPTIMEIO;
  std::memcpy(&payload[0], &objid, sizeof(objid));
  TEST_ASSERT_EQUAL(OK, send_user_frame(fd, NXSCOPE_HDRID_USER + 2, payload, sizeof(objid)));
  TEST_ASSERT_EQUAL(OK, recv_user_frame(fd, NXSCOPE_HDRID_USER + 2, resp, &n));
  TEST_ASSERT_EQUAL(6 + sizeof(uint64_t), n);
  std::memcpy(&objid, &resp[0], sizeof(objid));
  std::memcpy(&size, &resp[4], sizeof(size));
  TEST_ASSERT_EQUAL(NXSCOPE_UPTIMEIO, objid);
  TEST_ASSERT_EQUAL(sizeof(uint64_t), size);

  dawn_test_drain_pty_master(fd);
  close(fd);
  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  close(ttypfd);
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

static void test_proto_nxscope_serial_user_get_io_seek()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject desc(g_bin_nxscope_serial_seek);
  CProtoNxscopeSerial nxscope(desc);
  uint8_t set_payload[14];
  uint8_t seek_payload[10];
  uint8_t resp[128];
  size_t n;
  uint32_t objid;
  uint32_t offset;
  uint16_t size;
  int fd;
  int ttypfd;

  nxscope_setup(dummy1, fileio, nxscope, &fd, &ttypfd);

  objid = NXSCOPE_FILEIO1;
  offset = 2;
  size = 4;
  std::memcpy(&set_payload[0], &objid, sizeof(objid));
  std::memcpy(&set_payload[4], &offset, sizeof(offset));
  std::memcpy(&set_payload[8], &size, sizeof(size));
  std::memcpy(&set_payload[10], "WXYZ", 4);
  TEST_ASSERT_EQUAL(OK,
                    send_user_frame(fd, NXSCOPE_HDRID_USER + 1, set_payload, sizeof(set_payload)));

  offset = 1;
  size = 4;
  std::memcpy(&seek_payload[0], &objid, sizeof(objid));
  std::memcpy(&seek_payload[4], &offset, sizeof(offset));
  std::memcpy(&seek_payload[8], &size, sizeof(size));
  TEST_ASSERT_EQUAL(
    OK, send_user_frame(fd, NXSCOPE_HDRID_USER + 3, seek_payload, sizeof(seek_payload)));
  TEST_ASSERT_EQUAL(OK, recv_user_frame(fd, NXSCOPE_HDRID_USER + 2, resp, &n));
  TEST_ASSERT_EQUAL(6 + 4, n);
  std::memcpy(&objid, &resp[0], sizeof(objid));
  std::memcpy(&size, &resp[4], sizeof(size));
  TEST_ASSERT_EQUAL(NXSCOPE_FILEIO1, objid);
  TEST_ASSERT_EQUAL(4, size);
  TEST_ASSERT_EQUAL(0, std::memcmp(&resp[6], "0WXY", 4));

  dawn_test_drain_pty_master(fd);
  close(fd);
  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  close(ttypfd);
  dummy1.deinit();
  fileio.deinit();
  unlink(NXSCOPE_TEST_FILE);
}

//***************************************************************************
// Description: GET_IO of a 64 B IO fits the proto's own response buffer
// (the nxscope lib txbuf is CHINFO sized and would return -ENOBUFS).
//***************************************************************************

static void test_proto_nxscope_serial_user_get_io_large()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject descv3(g_cfg_uptime);
  CIOSysinfo uptime(descv3);
  CDescObject descv4(g_cfg_dummy16);
  CIODummy dummy16(descv4);
  CDescObject desc(g_bin_nxscope_serial_get);
  CProtoNxscopeSerial nxscope(desc);
  uint8_t resp[128];
  size_t n;
  uint32_t objid;
  uint16_t size;
  int32_t value;
  int fd;
  int ttypfd;
  int i;

  nxscope_setup(dummy1, fileio, nxscope, &fd, &ttypfd, &uptime, &dummy16);

  objid = NXSCOPE_DUMMYIO16;
  TEST_ASSERT_EQUAL(OK,
                    send_user_frame(fd, NXSCOPE_HDRID_USER + 2, (uint8_t *)&objid, sizeof(objid)));
  TEST_ASSERT_EQUAL(OK, recv_user_frame(fd, NXSCOPE_HDRID_USER + 2, resp, &n));
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

  dawn_test_drain_pty_master(fd);
  close(fd);
  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  close(ttypfd);
  dummy1.deinit();
  dummy16.deinit();
  uptime.deinit();
  fileio.deinit();
  unlink(NXSCOPE_TEST_FILE);
}

//***************************************************************************
// Description: GET_IO_SEEK past the end of the 8 B file is ACKed -EINVAL
// instead of shipping a stale tail; a window ending at EOF still works.
//***************************************************************************

static void test_proto_nxscope_serial_user_get_io_seek_eof()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject desc(g_bin_nxscope_serial_seek);
  CProtoNxscopeSerial nxscope(desc);
  uint8_t seek_payload[10];
  uint8_t resp[128];
  size_t n;
  uint32_t objid;
  uint32_t offset;
  uint16_t size;
  int fd;
  int ttypfd;

  nxscope_setup(dummy1, fileio, nxscope, &fd, &ttypfd);

  objid = NXSCOPE_FILEIO1;
  offset = 6;
  size = 4;
  std::memcpy(&seek_payload[0], &objid, sizeof(objid));
  std::memcpy(&seek_payload[4], &offset, sizeof(offset));
  std::memcpy(&seek_payload[8], &size, sizeof(size));
  TEST_ASSERT_EQUAL(
    OK, send_user_frame(fd, NXSCOPE_HDRID_USER + 3, seek_payload, sizeof(seek_payload)));
  TEST_ASSERT_EQUAL(-EINVAL, recv_ack(fd));

  offset = 4;
  std::memcpy(&seek_payload[4], &offset, sizeof(offset));
  TEST_ASSERT_EQUAL(
    OK, send_user_frame(fd, NXSCOPE_HDRID_USER + 3, seek_payload, sizeof(seek_payload)));
  TEST_ASSERT_EQUAL(OK, recv_user_frame(fd, NXSCOPE_HDRID_USER + 2, resp, &n));
  TEST_ASSERT_EQUAL(6 + 4, n);
  TEST_ASSERT_EQUAL(0, std::memcmp(&resp[6], "0000", 4));

  dawn_test_drain_pty_master(fd);
  close(fd);
  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  close(ttypfd);
  dummy1.deinit();
  fileio.deinit();
  unlink(NXSCOPE_TEST_FILE);
}

//***************************************************************************
// Description: SET_IO to a notify-capable virt bound as a stream channel
// re-enters the stream lock from the recv thread (virt notifies
// synchronously); the ACK must arrive and a following GET_IO must answer.
//***************************************************************************

static void test_proto_nxscope_serial_set_io_stream_virt()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject descv3(g_cfg_virt1);
  CIOVirt virt(descv3);
  CDescObject desc(g_bin_nxscope_serial_virt);
  CProtoNxscopeSerial nxscope(desc);
  uint8_t payload[10];
  uint8_t resp[128];
  size_t n;
  uint32_t objid;
  uint16_t size;
  int32_t value;
  int fd;
  int ttypfd;

  TEST_ASSERT_EQUAL(OK, virt.configure());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, virt.initialize(1, 1, true));
  nxscope_setup(dummy1, fileio, nxscope, &fd, &ttypfd, nullptr, nullptr, &virt, NXSCOPE_VIRTIO1);

  objid = NXSCOPE_VIRTIO1;
  size = sizeof(int32_t);
  value = 0x600d;
  std::memcpy(&payload[0], &objid, sizeof(objid));
  std::memcpy(&payload[4], &size, sizeof(size));
  std::memcpy(&payload[6], &value, sizeof(value));
  TEST_ASSERT_EQUAL(OK, send_user_frame(fd, NXSCOPE_HDRID_USER, payload, sizeof(payload)));
  TEST_ASSERT_EQUAL(OK, recv_ack(fd));

  TEST_ASSERT_EQUAL(OK, send_user_frame(fd, NXSCOPE_HDRID_USER + 2, payload, sizeof(objid)));
  TEST_ASSERT_EQUAL(OK, recv_user_frame(fd, NXSCOPE_HDRID_USER + 2, resp, &n));
  TEST_ASSERT_EQUAL(6 + sizeof(int32_t), n);
  std::memcpy(&value, &resp[6], sizeof(value));
  TEST_ASSERT_EQUAL(0x600d, value);

  dawn_test_drain_pty_master(fd);
  close(fd);
  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  close(ttypfd);
  dummy1.deinit();
  fileio.deinit();
  virt.deinit();
  unlink(NXSCOPE_TEST_FILE);
}

//***************************************************************************
// Description: a notify-capable stream channel whose notifier cannot be
// bound fails start() instead of silently downgrading to get-only.
//***************************************************************************

static void test_proto_nxscope_serial_start_notifier_fail()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_fileio_rw);
  CIOFile fileio(descv2);
  CDescObject descv3(g_cfg_notify1);
  CIODummyNotify notify1(descv3);
  CDescObject desc(g_bin_nxscope_serial_notify);
  CProtoNxscopeSerial nxscope(desc);
  int fd;
  int ttypfd;

  nxscope_setup(
    dummy1, fileio, nxscope, &fd, &ttypfd, nullptr, nullptr, &notify1, NXSCOPE_NOTIFYIO1, -EPERM);
  TEST_ASSERT_EQUAL(false, nxscope.hasThread());

  close(fd);
  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  close(ttypfd);
  dummy1.deinit();
  fileio.deinit();
  notify1.deinit();
  unlink(NXSCOPE_TEST_FILE);
}

extern "C"
{
  int test_proto_nxscope_serial()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_proto_nxscope_serial_user_set_io_simple);
    DAWN_RUN_TEST(test_proto_nxscope_serial_user_set_io_seek);
    DAWN_RUN_TEST(test_proto_nxscope_serial_user_get_io);
    DAWN_RUN_TEST(test_proto_nxscope_serial_user_get_io_seek);
    DAWN_RUN_TEST(test_proto_nxscope_serial_user_get_io_seek_eof);
    DAWN_RUN_TEST(test_proto_nxscope_serial_user_get_io_large);
    DAWN_RUN_TEST(test_proto_nxscope_serial_set_io_stream_virt);
    DAWN_RUN_TEST(test_proto_nxscope_serial_start_notifier_fail);
    return UNITY_END();
  }
}
