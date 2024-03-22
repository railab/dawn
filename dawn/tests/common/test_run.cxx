// dawn/tests/common/test_run.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <debug.h>

#include <cstdio>

#include "test_common.hxx"

extern "C"
{
  extern int test_common_handler();
  extern int test_common_bindable();
  extern int test_common_threaded();
  extern int test_common_objectid();
  extern int test_common_objectcfg();
  extern int test_common_object();
  extern int test_common_descriptor();
  extern int test_common_descswitch();
  extern int test_common_descobject();
}

static int (*test_array[])(void) = {
  test_common_handler,
  test_common_bindable,
  test_common_threaded,
  test_common_objectid,
  test_common_objectcfg,
  test_common_object,
  test_common_descriptor,
#ifdef CONFIG_DAWN_DESC_SWITCH
  test_common_descswitch,
#endif
  test_common_descobject,
  nullptr,
};

extern "C"
{
  int test_run_common()
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
