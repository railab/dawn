// dawn/tests/apps/test_run.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <cstdio>

#include "test_common.hxx"

extern "C"
{
  extern int test_apps_parseargs();
#ifdef CONFIG_DAWN_APPS_DESC_SOURCE_FILE
  extern int test_apps_file_load();
#endif
}

static int (*test_array[])(void) = {
  test_apps_parseargs,
#ifdef CONFIG_DAWN_APPS_DESC_SOURCE_FILE
  test_apps_file_load,
#endif
  nullptr,
};

extern "C"
{
  int test_run_apps()
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
