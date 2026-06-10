// dawn/tests/apps/test_file_load.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

#include "test_common.hxx"

#include "dawn_file_load.hxx"

//***************************************************************************
// Description: null path returns -EINVAL.
//***************************************************************************

static void test_apps_file_load_null_path()
{
  uint32_t *bin = nullptr;
  size_t len = 0;
  int ret;

  ret = dawn_file_load_descriptor(nullptr, 4096, &bin, &len);
  TEST_ASSERT(ret < 0);
  TEST_ASSERT_EQUAL(nullptr, bin);
  TEST_ASSERT_EQUAL(0u, len);
}

//***************************************************************************
// Description: null outBin returns -EINVAL.
//***************************************************************************

static void test_apps_file_load_null_outbin()
{
  size_t len = 0;
  int ret;

  ret = dawn_file_load_descriptor("/nonexistent", 4096, nullptr, &len);
  TEST_ASSERT(ret < 0);
}

//***************************************************************************
// Description: null outLen returns -EINVAL.
//***************************************************************************

static void test_apps_file_load_null_outlen()
{
  uint32_t *bin = nullptr;
  int ret;

  ret = dawn_file_load_descriptor("/nonexistent", 4096, &bin, nullptr);
  TEST_ASSERT(ret < 0);
}

//***************************************************************************
// Description: non-existent file returns error.
//***************************************************************************

static void test_apps_file_load_missing_file()
{
  uint32_t *bin = nullptr;
  size_t len = 0;
  int ret;

  ret = dawn_file_load_descriptor("/nonexistent_file_xyz", 4096, &bin, &len);
  TEST_ASSERT(ret < 0);
  TEST_ASSERT_EQUAL(nullptr, bin);
  TEST_ASSERT_EQUAL(0u, len);
}

//***************************************************************************
// Description: empty file fails (too small, not multiple of 4).
//***************************************************************************

static void test_apps_file_load_empty_file()
{
  const char *path = "/tmp/dawn_test_empty.bin";
  uint32_t *bin = nullptr;
  size_t len = 0;
  int ret;
  int fd;

  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(fd >= 0);
  close(fd);

  ret = dawn_file_load_descriptor(path, 4096, &bin, &len);
  TEST_ASSERT(ret < 0);
  TEST_ASSERT_EQUAL(nullptr, bin);

  unlink(path);
}

//***************************************************************************
// Description: file too large for maxSize returns -EFBIG.
//***************************************************************************

static void test_apps_file_load_too_large()
{
  const char *path = "/tmp/dawn_test_large.bin";
  uint32_t *bin = nullptr;
  size_t len = 0;
  uint8_t buf[100] = {};
  int ret;
  int fd;

  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(fd >= 0);
  write(fd, buf, sizeof(buf));
  close(fd);

  ret = dawn_file_load_descriptor(path, 50, &bin, &len);
  TEST_ASSERT_EQUAL(-EFBIG, ret);
  TEST_ASSERT_EQUAL(nullptr, bin);

  unlink(path);
}

//***************************************************************************
// Description: file with invalid magic returns error.
//***************************************************************************

static void test_apps_file_load_invalid_magic()
{
  const char *path = "/tmp/dawn_test_bad_magic.bin";
  uint32_t *bin = nullptr;
  size_t len = 0;
  uint32_t buf[4] = {0xDEADBEEF, 1, 0x02030A0D, 0};
  int ret;
  int fd;

  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(fd >= 0);
  write(fd, buf, sizeof(buf));
  close(fd);

  ret = dawn_file_load_descriptor(path, 4096, &bin, &len);
  TEST_ASSERT(ret < 0);
  TEST_ASSERT_EQUAL(nullptr, bin);

  unlink(path);
}

extern "C"
{
  int test_apps_file_load()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_apps_file_load_null_path);
    DAWN_RUN_TEST(test_apps_file_load_null_outbin);
    DAWN_RUN_TEST(test_apps_file_load_null_outlen);
    DAWN_RUN_TEST(test_apps_file_load_missing_file);
    DAWN_RUN_TEST(test_apps_file_load_empty_file);
    DAWN_RUN_TEST(test_apps_file_load_too_large);
    DAWN_RUN_TEST(test_apps_file_load_invalid_magic);

    return UNITY_END();
  }
}
