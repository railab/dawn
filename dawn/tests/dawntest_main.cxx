// dawn/tests/dawntest_main.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <debug.h>
#include <sys/boardctl.h>

#include <cstddef>
#include <cstdio>
#include <cstring>

#include "dawn/debug.hxx"
#include "dawn/porting/board.hxx"
#include "dawn/porting/config.hxx"
#include "include/test_common.hxx"
#include "include/test_registry.hxx"

#ifndef CONFIG_BOARDCTL_POWEROFF
#  error CONFIG_BOARDCTL_POWEROFF is required
#endif

extern "C"
{
  extern int test_run_porting();
  extern int test_run_common();
  extern int test_run_dev();
  extern int test_run_io();
  extern int test_run_prog();
  extern int test_run_proto();
  extern int test_run_dawn();
}

static const test_registry_entry_s g_test_registry[] = {
  {"porting.run", "porting", test_run_porting},
  {"common.run", "common", test_run_common},
  {"dev.run", "dev", test_run_dev},
#ifdef CONFIG_DAWN_IO
  {"io.run", "io", test_run_io},
#endif
#ifdef CONFIG_DAWN_PROG
  {"prog.run", "prog", test_run_prog},
#endif
#ifdef CONFIG_DAWN_PROTO
  {"proto.run", "proto", test_run_proto},
#endif
  {"dawn.run", "dawn", test_run_dawn},
};

// make unity happy with setUp and tearDown

void setUp()
{
}

void tearDown()
{
}

// dawntest_main - entry point for Dawn project tests

extern "C"
{
  int main(int argc, FAR char *argv[])
  {
    test_filter_s filter;
    int ret = 0;
    size_t i = 0;
    size_t test_count = sizeof(g_test_registry) / sizeof(g_test_registry[0]);
    bool fail = false;
    bool test_ran = false;

    test_filter_init(&filter);
    ret = test_filter_parse_args(argc, argv, &filter);
    if (ret < 0)
      {
        test_registry_print_usage(argv[0]);
        return 1;
      }

    if (filter.help)
      {
        test_registry_print_usage(argv[0]);
        return 0;
      }

    if (filter.list_only)
      {
        test_registry_list(g_test_registry, test_count, &filter);
        return 0;
      }

    test_registry_set_active_filter(&filter);
    test_registry_reset_case_matches();

    {
      // Board initialize

      ret = dawn_board_init();
      if (ret < 0)
        {
          DAWNERR("ERROR: dawn_board_init failed %d\n", ret);
          fail = true;
          ret = 1;
          goto out;
        }
    }

    // Run tests

    for (i = 0; i < test_count; i += 1)
      {
        if (filter.module != nullptr && strcmp(g_test_registry[i].module, filter.module) != 0)
          {
            continue;
          }

        test_ran = true;
        ret = g_test_registry[i].func();
        if (ret != 0)
          {
            fail = true;
#ifdef CONFIG_DAWN_TEST_EXIT_ON_FAIL
            printf("Force exit on the first fail!\n");
            goto out;
#endif
          }
      }

  out:

    if (filter.test != nullptr || filter.prefix != nullptr)
      {
        test_ran = test_registry_has_case_matches();
      }

    if (!test_ran && ret == 0)
      {
        printf("No tests matched the provided filters.\n");
        fail = true;
      }

    if (fail)
      {
        printf("Test result: FAILED!\n");
        ret = 1;
      }
    else
      {
        printf("Test result: PASSED!\n");
        ret = 0;
      }

#ifndef CONFIG_SYSTEM_NSH
    // Close simulation

    boardctl(BOARDIOC_POWEROFF, 0);
#endif

    return ret;
  }
}
