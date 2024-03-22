// dawn/tests/include/test_common.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "test_registry.hxx"

#include <fcntl.h>
#include <unistd.h>

#define UNITY_EXCLUDE_MATH_H
#include <testing/unity.h>

#define DAWN_TEST_SEPARATOR()                                               \
  do                                                                        \
    {                                                                       \
      printf("\n======================================================\n"); \
      printf("%s\n", __FILE__);                                             \
      printf("======================================================\n\n"); \
    }                                                                       \
  while (0);

#define DAWN_RUN_TEST(test_func)                               \
  do                                                           \
    {                                                          \
      if (test_registry_should_run_case(__FILE__, #test_func)) \
        {                                                      \
          RUN_TEST(test_func);                                 \
        }                                                      \
    }                                                          \
  while (0);

static inline void dawn_test_drain_pty_master(int fd)
{
  int flags;
  uint8_t buf[128];
  ssize_t n;

  if (fd < 0)
    {
      return;
    }

  flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    {
      return;
    }

  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  do
    {
      n = read(fd, buf, sizeof(buf));
    }
  while (n > 0);

  fcntl(fd, F_SETFL, flags);
}
