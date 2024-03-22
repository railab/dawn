// dawn/tests/proto/test_nxscope_serial.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <cstring>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "dawn/io/dummy.hxx"
#include "dawn/io/fileio.hxx"
#include "dawn/proto/nxscope/serial.hxx"
#include "logging/nxscope/nxscope.h"
#include "logging/nxscope/nxscope_proto.h"
#include "test_common.hxx"

using namespace dawn;

#ifndef CONFIG_DAWN_IO_FILE
#  error CONFIG_DAWN_IO_FILE must be enabled
#endif

static constexpr auto NXSCOPE_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 1);
static constexpr auto NXSCOPE_FILEIO1 = CIOFile::objectId(1);
static constexpr auto NXSCOPE_TEST_FILE = "/tmp/nxset.bin";

static uint32_t g_cfg_dummy1[] = {
  NXSCOPE_DUMMYIO1,
  1,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
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

// Configure + init + bind dummy + fileio, configure + start nxscope.
// Caller closes the PTY fds and stops nxscope.

static void nxscope_setup(CIODummy &dummy,
                          CIOFile &fileio,
                          CProtoNxscopeSerial &nxscope,
                          int *out_pty_fd,
                          int *out_ttyp_fd)
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
  TEST_ASSERT_EQUAL(OK, nxscope.init());
  TEST_ASSERT_EQUAL(OK, nxscope.start());
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

extern "C"
{
  int test_proto_nxscope_serial()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_proto_nxscope_serial_user_set_io_simple);
    DAWN_RUN_TEST(test_proto_nxscope_serial_user_set_io_seek);
    return UNITY_END();
  }
}
