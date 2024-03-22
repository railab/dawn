// dawn/tests/io/test_fileio.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/ddata.hxx"
#include "dawn/io/fileio.hxx"
#include "test_common.hxx"

#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>

using namespace dawn;

static constexpr auto TEST_FILE_PATH = "/tmp/dfio.bin";

//***************************************************************************
// Description: valid path /tmp/dfio.bin, read-only permission
//
// Path "/tmp/dfio.bin" packed as little-endian uint32 words (4 words):
//   Word 0: '/', 't', 'm', 'p'  = 0x706d742f
//   Word 1: '/', 'd', 'f', 'i'  = 0x6966642f
//   Word 2: 'o', '.', 'b', 'i'  = 0x69622e6f
//   Word 3: 'n', '\0','\0','\0' = 0x0000006e
//***************************************************************************

static uint32_t g_cfg_fileio_read[] = {
  CIOFile::objectId(0),
  2,
  CIOFile::cfgIdPath(4),
  0x706d742f,
  0x6966642f,
  0x69622e6f,
  0x0000006e,
  CIOFile::cfgIdPerm(),
  CIOFile::IO_FILE_PERM_READ,
};

//***************************************************************************
// Description: valid path /tmp/dfio.bin, write-only permission
//***************************************************************************

static uint32_t g_cfg_fileio_write[] = {
  CIOFile::objectId(2),
  2,
  CIOFile::cfgIdPath(4),
  0x706d742f,
  0x6966642f,
  0x69622e6f,
  0x0000006e,
  CIOFile::cfgIdPerm(),
  CIOFile::IO_FILE_PERM_WRITE,
};

//***************************************************************************
// Description: valid path /tmp/dfio.bin, read-write permission
//***************************************************************************

static uint32_t g_cfg_fileio_rw[] = {
  CIOFile::objectId(3),
  2,
  CIOFile::cfgIdPath(4),
  0x706d742f,
  0x6966642f,
  0x69622e6f,
  0x0000006e,
  CIOFile::cfgIdPerm(),
  CIOFile::IO_FILE_PERM_RW,
};

//***************************************************************************
// Description: valid path /tmp/dfio.bin, write-once permission
//***************************************************************************

static uint32_t g_cfg_fileio_write_once[] = {
  CIOFile::objectId(6),
  2,
  CIOFile::cfgIdPath(4),
  0x706d742f,
  0x6966642f,
  0x69622e6f,
  0x0000006e,
  CIOFile::cfgIdPerm(),
  CIOFile::IO_FILE_PERM_WRITE_ONCE,
};

//***************************************************************************
// Description: invalid path /etc/test (not in whitelist)
//
// Path "/etc/test" packed as little-endian uint32 words (4 words, min_words=4):
//   Word 0: '/', 'e', 't', 'c'  = 0x6374652f
//   Word 1: '/', 't', 'e', 's'  = 0x7365742f
//   Word 2: 't', '\0','\0','\0' = 0x00000074
//   Word 3: '\0','\0','\0','\0' = 0x00000000
//***************************************************************************

static uint32_t g_cfg_invalid_dir[] = {
  CIOFile::objectId(4),
  1,
  CIOFile::cfgIdPath(4),
  0x6374652f,
  0x7365742f,
  0x00000074,
  0x00000000,
};

//***************************************************************************
// Description: path traversal attack /tmp/../etc (must be rejected)
//
// Path "/tmp/../etc" packed as little-endian uint32 words (4 words,
// min_words=4):
//   Word 0: '/', 't', 'm', 'p'  = 0x706d742f
//   Word 1: '/', '.', '.', '/'  = 0x2f2e2e2f
//   Word 2: 'e', 't', 'c', '\0' = 0x00637465
//   Word 3: '\0','\0','\0','\0' = 0x00000000
//***************************************************************************

static uint32_t g_cfg_traversal[] = {
  CIOFile::objectId(5),
  1,
  CIOFile::cfgIdPath(4),
  0x706d742f,
  0x2f2e2e2f,
  0x00637465,
  0x00000000,
};

// mount tmpfs at /tmp for file-based tests

static void setup_tmp()
{
  mkdir("/tmp", 0777);
  mount(NULL, "/tmp", "tmpfs", 0, NULL);
}

// write the supplied buffer to TEST_FILE_PATH (creates/truncs).

static void fileio_seed(const void *data, size_t n)
{
  int fd;

  fd = open(TEST_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(fd >= 0);
  TEST_ASSERT(write(fd, data, n) == (ssize_t)n);
  close(fd);
}

// configure + init the supplied IO; assert OK on each step.

static void fileio_open_io(CIOFile &io)
{
  TEST_ASSERT_EQUAL(OK, io.configure());
  TEST_ASSERT_EQUAL(OK, io.init());
}

// read TEST_FILE_PATH and assert that contents match expected.

static void fileio_assert_file(const void *expected, size_t n)
{
  char buf[64];
  int fd;
  ssize_t nread;

  TEST_ASSERT(n <= sizeof(buf));

  fd = open(TEST_FILE_PATH, O_RDONLY);
  TEST_ASSERT(fd >= 0);
  nread = read(fd, buf, sizeof(buf));
  TEST_ASSERT_EQUAL((ssize_t)n, nread);
  TEST_ASSERT_EQUAL(0, std::memcmp(buf, expected, n));
  close(fd);
}

//***************************************************************************
// Description: paths outside the directory whitelist are rejected.
//***************************************************************************

static void test_io_fileio_path_outside_whitelist()
{
  CDescObject desc_dir(g_cfg_invalid_dir);
  CIOFile io_dir(desc_dir);

  TEST_ASSERT_EQUAL(-EACCES, io_dir.configure());
}

//***************************************************************************
// Description: paths containing ".." are rejected as traversal attempts.
//***************************************************************************

static void test_io_fileio_path_traversal()
{
  CDescObject desc_trav(g_cfg_traversal);
  CIOFile io_trav(desc_trav);

  TEST_ASSERT_EQUAL(-EACCES, io_trav.configure());
}

//***************************************************************************
// Description: a read-only IO reports the right capability flags.
//***************************************************************************

static void test_io_fileio_read_properties()
{
  CDescObject desc(g_cfg_fileio_read);
  CIOFile io(desc);
  static const char test_data[] = "DAWN FILE IO";

  setup_tmp();
  fileio_seed(test_data, sizeof(test_data));

  fileio_open_io(io);

  TEST_ASSERT_EQUAL(true, io.isRead());
  TEST_ASSERT_EQUAL(false, io.isWrite());
  TEST_ASSERT_EQUAL(false, io.isNotify());
  TEST_ASSERT_EQUAL(false, io.isBatch());
  TEST_ASSERT_EQUAL(true, io.isSeekable());
  TEST_ASSERT_EQUAL(sizeof(test_data), io.getDataSize());
  TEST_ASSERT_EQUAL(sizeof(test_data), io.getDataDim());

  io.deinit();
  unlink(TEST_FILE_PATH);
}

//***************************************************************************
// Description: read-only IO returns the file contents through getData().
//***************************************************************************

static void test_io_fileio_read_contents()
{
  CDescObject desc(g_cfg_fileio_read);
  CIOFile io(desc);
  static const char test_data[] = "DAWN FILE IO";
  io_ddata_t *buf;
  uint8_t *ptr;

  setup_tmp();
  fileio_seed(test_data, sizeof(test_data));
  fileio_open_io(io);

  buf = io.ddata_alloc(1, io.getDataSize());
  TEST_ASSERT(buf != nullptr);

  TEST_ASSERT_EQUAL(OK, io.getData(*buf, 1, 0));

  ptr = static_cast<uint8_t *>(buf->getDataPtr());
  TEST_ASSERT_EQUAL(0, std::memcmp(ptr, test_data, sizeof(test_data)));

  free(buf);
  io.deinit();
  unlink(TEST_FILE_PATH);
}

//***************************************************************************
// Description: a write-only IO reports the right capability flags and zero
// size before any write.
//***************************************************************************

static void test_io_fileio_write_properties()
{
  CDescObject desc(g_cfg_fileio_write);
  CIOFile io(desc);

  setup_tmp();
  unlink(TEST_FILE_PATH);
  fileio_open_io(io);

  TEST_ASSERT_EQUAL(false, io.isRead());
  TEST_ASSERT_EQUAL(true, io.isWrite());
  TEST_ASSERT_EQUAL(true, io.isSeekable());
  TEST_ASSERT_EQUAL(0u, io.getDataSize());
  TEST_ASSERT_EQUAL(0u, io.getDataDim());

  io.deinit();
  unlink(TEST_FILE_PATH);
}

//***************************************************************************
// Description: a write-only IO writes data to the underlying file.
//***************************************************************************

static void test_io_fileio_write_contents()
{
  CDescObject desc(g_cfg_fileio_write);
  CIOFile io(desc);
  static const char write_data[] = "WRITE TEST";
  io_ddata_t *buf;

  setup_tmp();
  unlink(TEST_FILE_PATH);
  fileio_open_io(io);

  // ddata_alloc() requires dim > 0 (readable IO), so allocate the buffer
  // directly for write-only IO.

  buf = new io_ddata_t(1, sizeof(write_data));
  TEST_ASSERT(buf != nullptr);
  std::memcpy(buf->getDataPtr(0), write_data, sizeof(write_data));

  TEST_ASSERT_EQUAL(OK, io.setData(*buf, 0));

  delete buf;
  io.deinit();

  fileio_assert_file(write_data, sizeof(write_data));
  unlink(TEST_FILE_PATH);
}

//***************************************************************************
// Description: read-write IO reads slices from arbitrary offsets.
//***************************************************************************

static void test_io_fileio_seek_read()
{
  CDescObject desc(g_cfg_fileio_rw);
  CIOFile io(desc);
  static const char file_data[] = "ABCDEFGHIJ";
  io_ddata_t *rbuf;
  uint8_t *ptr;

  setup_tmp();
  fileio_seed(file_data, sizeof(file_data));
  fileio_open_io(io);

  rbuf = io.ddata_alloc(1, 4);
  TEST_ASSERT(rbuf != nullptr);

  TEST_ASSERT_EQUAL(OK, io.getData(*rbuf, 1, 0));
  ptr = static_cast<uint8_t *>(rbuf->getDataPtr());
  TEST_ASSERT_EQUAL('A', ptr[0]);
  TEST_ASSERT_EQUAL('D', ptr[3]);

  TEST_ASSERT_EQUAL(OK, io.getData(*rbuf, 1, 4));
  ptr = static_cast<uint8_t *>(rbuf->getDataPtr());
  TEST_ASSERT_EQUAL('E', ptr[0]);
  TEST_ASSERT_EQUAL('H', ptr[3]);

  free(rbuf);
  io.deinit();
  unlink(TEST_FILE_PATH);
}

//***************************************************************************
// Description: reading at an offset past EOF returns -EINVAL.
//***************************************************************************

static void test_io_fileio_seek_read_past_eof()
{
  CDescObject desc(g_cfg_fileio_rw);
  CIOFile io(desc);
  static const char file_data[] = "ABCDEFGHIJ";
  io_ddata_t *rbuf;

  setup_tmp();
  fileio_seed(file_data, sizeof(file_data));
  fileio_open_io(io);

  rbuf = io.ddata_alloc(1, 4);
  TEST_ASSERT(rbuf != nullptr);

  TEST_ASSERT_EQUAL(-EINVAL, io.getData(*rbuf, 1, sizeof(file_data)));

  free(rbuf);
  io.deinit();
  unlink(TEST_FILE_PATH);
}

//***************************************************************************
// Description: read-write IO writes a slice at an offset and the change is
// visible on the next read.
//***************************************************************************

static void test_io_fileio_seek_write()
{
  CDescObject desc(g_cfg_fileio_rw);
  CIOFile io(desc);
  static const char file_data[] = "ABCDEFGHIJ";
  io_ddata_t *wbuf;
  io_ddata_t *rbuf;
  uint8_t *ptr;

  setup_tmp();
  fileio_seed(file_data, sizeof(file_data));
  fileio_open_io(io);

  wbuf = io.ddata_alloc(1, 2);
  TEST_ASSERT(wbuf != nullptr);
  ptr = static_cast<uint8_t *>(wbuf->getDataPtr(0));
  ptr[0] = 'X';
  ptr[1] = 'Y';

  TEST_ASSERT_EQUAL(OK, io.setData(*wbuf, 2));

  rbuf = io.ddata_alloc(1, 4);
  TEST_ASSERT(rbuf != nullptr);

  TEST_ASSERT_EQUAL(OK, io.getData(*rbuf, 1, 0));
  ptr = static_cast<uint8_t *>(rbuf->getDataPtr());
  TEST_ASSERT_EQUAL('A', ptr[0]);
  TEST_ASSERT_EQUAL('B', ptr[1]);
  TEST_ASSERT_EQUAL('X', ptr[2]);
  TEST_ASSERT_EQUAL('Y', ptr[3]);

  free(rbuf);
  free(wbuf);
  io.deinit();
  unlink(TEST_FILE_PATH);
}

//***************************************************************************
// Description: write-once IO accepts the first write and tracks size.
//***************************************************************************

static void test_io_fileio_write_once_first_write()
{
  CDescObject desc(g_cfg_fileio_write_once);
  CIOFile io(desc);
  io_ddata_t *buf;
  uint8_t *ptr;

  setup_tmp();
  unlink(TEST_FILE_PATH);
  fileio_open_io(io);

  buf = new io_ddata_t(1, 4);
  TEST_ASSERT(buf != nullptr);
  ptr = static_cast<uint8_t *>(buf->getDataPtr(0));
  ptr[0] = 'A';
  ptr[1] = 'B';
  ptr[2] = 'C';
  ptr[3] = 'D';

  TEST_ASSERT_EQUAL(OK, io.setData(*buf, 0));
  TEST_ASSERT_EQUAL(4u, io.getDataSize());

  delete buf;
  io.deinit();

  fileio_assert_file("ABCD", 4);
  unlink(TEST_FILE_PATH);
}

//***************************************************************************
// Description: write-once IO rejects a second setData() with -EPERM.
//***************************************************************************

static void test_io_fileio_write_once_second_write_rejected()
{
  CDescObject desc(g_cfg_fileio_write_once);
  CIOFile io(desc);
  io_ddata_t *buf;
  uint8_t *ptr;

  setup_tmp();
  unlink(TEST_FILE_PATH);
  fileio_open_io(io);

  buf = new io_ddata_t(1, 4);
  TEST_ASSERT(buf != nullptr);
  ptr = static_cast<uint8_t *>(buf->getDataPtr(0));
  ptr[0] = 'A';
  ptr[1] = 'B';
  ptr[2] = 'C';
  ptr[3] = 'D';

  TEST_ASSERT_EQUAL(OK, io.setData(*buf, 0));

  ptr[0] = 'W';
  ptr[1] = 'X';
  ptr[2] = 'Y';
  ptr[3] = 'Z';
  TEST_ASSERT_EQUAL(-EPERM, io.setData(*buf, 0));

  delete buf;
  io.deinit();

  // First write must remain on disk untouched.

  fileio_assert_file("ABCD", 4);
  unlink(TEST_FILE_PATH);
}

//***************************************************************************
// Description: write-once IO rejects writes when the file already exists
// with non-empty contents (locked at init).
//***************************************************************************

static void test_io_fileio_write_once_existing_file_locked()
{
  CDescObject desc(g_cfg_fileio_write_once);
  CIOFile io(desc);
  io_ddata_t *buf;
  uint8_t *ptr;

  setup_tmp();
  fileio_seed("LOCK", 4);
  fileio_open_io(io);

  buf = new io_ddata_t(1, 2);
  TEST_ASSERT(buf != nullptr);
  ptr = static_cast<uint8_t *>(buf->getDataPtr(0));
  ptr[0] = 'N';
  ptr[1] = 'O';

  TEST_ASSERT_EQUAL(-EPERM, io.setData(*buf, 0));

  delete buf;
  io.deinit();
  unlink(TEST_FILE_PATH);
}

//***************************************************************************
// Description: rewriting from offset 0 truncates any stale tail past the
// new write length.
//***************************************************************************

static void test_io_fileio_rewrite_truncates_tail()
{
  CDescObject desc(g_cfg_fileio_rw);
  CIOFile io(desc);
  static const char initial_data[] = "LONGER-DATA";
  static const char short_data[] = "SHORT";
  io_ddata_t *wbuf;

  setup_tmp();
  fileio_seed(initial_data, sizeof(initial_data) - 1);
  fileio_open_io(io);

  wbuf = new io_ddata_t(1, sizeof(short_data) - 1);
  TEST_ASSERT(wbuf != nullptr);
  std::memcpy(wbuf->getDataPtr(0), short_data, sizeof(short_data) - 1);

  TEST_ASSERT_EQUAL(OK, io.setData(*wbuf, 0));
  TEST_ASSERT_EQUAL(sizeof(short_data) - 1, io.getDataSize());

  delete wbuf;
  io.deinit();

  fileio_assert_file(short_data, sizeof(short_data) - 1);
  unlink(TEST_FILE_PATH);
}

extern "C"
{
  int test_io_fileio()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_fileio_path_outside_whitelist);
    DAWN_RUN_TEST(test_io_fileio_path_traversal);

    DAWN_RUN_TEST(test_io_fileio_read_properties);
    DAWN_RUN_TEST(test_io_fileio_read_contents);

    DAWN_RUN_TEST(test_io_fileio_write_properties);
    DAWN_RUN_TEST(test_io_fileio_write_contents);

    DAWN_RUN_TEST(test_io_fileio_seek_read);
    DAWN_RUN_TEST(test_io_fileio_seek_read_past_eof);
    DAWN_RUN_TEST(test_io_fileio_seek_write);

    DAWN_RUN_TEST(test_io_fileio_write_once_first_write);
    DAWN_RUN_TEST(test_io_fileio_write_once_second_write_rejected);
    DAWN_RUN_TEST(test_io_fileio_write_once_existing_file_locked);

    DAWN_RUN_TEST(test_io_fileio_rewrite_truncates_tail);

    return UNITY_END();
  }
}
