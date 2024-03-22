// dawn/tests/dev/test_run.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <debug.h>

#include <cstdio>

#include "test_common.hxx"

extern "C"
{
  // dev management

  int test_dev_descriptor();
  int test_dev_inspector();
}

static int (*test_array[])(void) = {
  // dev management

  test_dev_descriptor,
#ifdef CONFIG_DAWN_INSPECT
  test_dev_inspector,
#endif

  nullptr,
};

extern "C"
{
  int test_run_dev()
  {
    int ret = 0;
    int i = 0;
    bool fail = false;

    DAWN_TEST_SEPARATOR();

    for (i = 0; test_array[i] != nullptr; i += 1)
      {
        ret = test_array[i]();
        if (ret != 0)
          {
            fail = true;
          }
#ifdef CONFIG_DAWN_TEST_EXIT_ON_FAIL
        if (ret != 0)
          {
            printf("Force exit on the first fail!\n");
            goto errout;
          }
#endif
      }

#ifdef CONFIG_DAWN_TEST_EXIT_ON_FAIL
  errout:
#endif
    return fail ? -1 : 0;
  }
}
