/*
 * boards/common/src/fake_files.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <nuttx/fs/fs.h>

#include "dawn/fake_files.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FAKE_FILE_INITIAL_DATA "initial data"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: write_fake_file
 *
 * Description:
 *   Create or overwrite a single file at the given path and seed it with
 *   the canonical FAKE_FILE_INITIAL_DATA payload. Pure POSIX so it works
 *   on any board with a writable filesystem.
 *
 ****************************************************************************/

static int write_fake_file(const char *path)
{
  ssize_t n;
  size_t len;
  int fd;
  int ret;

  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
    {
      ret = -errno;
      syslog(LOG_ERR, "ERROR: fake_files: open %s failed: %d\n", path, ret);
      return ret;
    }

  len = strlen(FAKE_FILE_INITIAL_DATA);
  n   = write(fd, FAKE_FILE_INITIAL_DATA, len);
  if (n < 0)
    {
      ret = -errno;
      syslog(LOG_ERR, "ERROR: fake_files: write %s failed: %d\n", path, ret);
      close(fd);
      return ret;
    }

  close(fd);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dawn_fake_files_init
 *
 * Description:
 *   Pre-populate the configured tmpfs mount point with the set of fake
 *   files selected via Kconfig.
 *
 ****************************************************************************/

int dawn_fake_files_init(void)
{
  int last_err = OK;
  int ret;

#ifdef CONFIG_DAWN_FAKE_FILE_RO
  ret = write_fake_file(CONFIG_DAWN_FAKE_FILES_MOUNTPOINT "/some_file_ro.txt");
  if (ret < 0)
    {
      last_err = ret;
    }
#endif

#ifdef CONFIG_DAWN_FAKE_FILE_RW
  ret = write_fake_file(CONFIG_DAWN_FAKE_FILES_MOUNTPOINT "/some_file_rw.txt");
  if (ret < 0)
    {
      last_err = ret;
    }
#endif

  return last_err;
}
