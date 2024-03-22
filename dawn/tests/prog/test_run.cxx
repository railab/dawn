// dawn/tests/prog/test_run.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <debug.h>

#include <cstdio>

#include "test_common.hxx"

extern "C"
{
  // Common

  int test_prog_factory();
  int test_prog_handler();
  int test_prog_common();

  // Program classes

#ifdef CONFIG_DAWN_PROG_PROCESS
  int test_prog_process();
#endif
#ifdef CONFIG_DAWN_PROG_STATS_MIN
  int test_prog_stats_min();
#endif
#ifdef CONFIG_DAWN_PROG_STATS_MAX
  int test_prog_stats_max();
#endif
#ifdef CONFIG_DAWN_PROG_STATS_AVG
  int test_prog_stats_avg();
#endif
#ifdef CONFIG_DAWN_PROG_STATS_SUM
  int test_prog_stats_sum();
#endif
#ifdef CONFIG_DAWN_PROG_STATS_COUNT
  int test_prog_stats_count();
#endif
#ifdef CONFIG_DAWN_PROG_STATS_RMS
  int test_prog_stats_rms();
#endif
#ifdef CONFIG_DAWN_PROG_ADJUST
  int test_prog_adjust();
#endif
#ifdef CONFIG_DAWN_PROG_GATEWAY
  int test_prog_gateway();
#endif
#ifdef CONFIG_DAWN_PROG_LATEST
  int test_prog_latest();
#endif
#ifdef CONFIG_DAWN_PROG_REDIRECT
  int test_prog_redirect();
#endif
#ifdef CONFIG_DAWN_PROG_BUFFER
  int test_prog_buffer();
#endif
#ifdef CONFIG_DAWN_PROG_MOVING_AVG
  int test_prog_moving_avg();
#endif
#ifdef CONFIG_DAWN_PROG_IIR_FILTER
  int test_prog_iir_filter();
#endif
#ifdef CONFIG_DAWN_PROG_THRESHOLD_ANY
  int test_prog_threshold();
#endif
#ifdef CONFIG_DAWN_PROG_SAMPLING
  int test_prog_sampling();
#endif
#ifdef CONFIG_DAWN_PROG_SEQUENCER
  int test_prog_sequencer();
#endif
#ifdef CONFIG_DAWN_PROG_BITSPLIT
  int test_prog_bitsplit();
#endif
#ifdef CONFIG_DAWN_PROG_TOGGLE
  int test_prog_toggle();
#endif
#ifdef CONFIG_DAWN_PROG_COUNTER
  int test_prog_counter();
#endif
#ifdef CONFIG_DAWN_PROG_SWITCH
  int test_prog_switch();
#endif
#ifdef CONFIG_DAWN_PROG_EXPRESSION
  int test_prog_expression();
#endif
#ifdef CONFIG_DAWN_PROG_SELECTOR
  int test_prog_selector();
#endif
#ifdef CONFIG_DAWN_PROG_BITPACK
  int test_prog_bitpack();
#endif
#ifdef CONFIG_DAWN_PROG_VECPACK
  int test_prog_vecpack();
#endif
#ifdef CONFIG_DAWN_PROG_VECSPLIT
  int test_prog_vecsplit();
#endif
#ifdef CONFIG_DAWN_PROG_MANYTOONE
  int test_prog_manytoone();
#endif
#ifdef CONFIG_DAWN_PROG_ONETOMANY
  int test_prog_onetomany();
#endif
#ifdef CONFIG_DAWN_PROG_IOMUX
  int test_prog_iomux();
#endif
#ifdef CONFIG_DAWN_PROG_IODEMUX
  int test_prog_iodemux();
#endif
}

static int (*test_array[])(void) = {
  // Common

  test_prog_factory,
  test_prog_handler,
  test_prog_common,

// Program classes

#ifdef CONFIG_DAWN_PROG_PROCESS
  test_prog_process,
#endif
#ifdef CONFIG_DAWN_PROG_STATS_MIN
  test_prog_stats_min,
#endif
#ifdef CONFIG_DAWN_PROG_STATS_MAX
  test_prog_stats_max,
#endif
#ifdef CONFIG_DAWN_PROG_STATS_AVG
  test_prog_stats_avg,
#endif
#ifdef CONFIG_DAWN_PROG_STATS_SUM
  test_prog_stats_sum,
#endif
#ifdef CONFIG_DAWN_PROG_STATS_COUNT
  test_prog_stats_count,
#endif
#ifdef CONFIG_DAWN_PROG_STATS_RMS
  test_prog_stats_rms,
#endif
#ifdef CONFIG_DAWN_PROG_ADJUST
  test_prog_adjust,
#endif
#ifdef CONFIG_DAWN_PROG_GATEWAY
  test_prog_gateway,
#endif
#ifdef CONFIG_DAWN_PROG_LATEST
  test_prog_latest,
#endif
#ifdef CONFIG_DAWN_PROG_REDIRECT
  test_prog_redirect,
#endif
#ifdef CONFIG_DAWN_PROG_BUFFER
  test_prog_buffer,
#endif
#ifdef CONFIG_DAWN_PROG_MOVING_AVG
  test_prog_moving_avg,
#endif
#ifdef CONFIG_DAWN_PROG_IIR_FILTER
  test_prog_iir_filter,
#endif
#ifdef CONFIG_DAWN_PROG_THRESHOLD_ANY
  test_prog_threshold,
#endif
#ifdef CONFIG_DAWN_PROG_SAMPLING
  test_prog_sampling,
#endif
#ifdef CONFIG_DAWN_PROG_BITSPLIT
  test_prog_bitsplit,
#endif
#ifdef CONFIG_DAWN_PROG_TOGGLE
  test_prog_toggle,
#endif
#ifdef CONFIG_DAWN_PROG_COUNTER
  test_prog_counter,
#endif
#ifdef CONFIG_DAWN_PROG_SWITCH
  test_prog_switch,
#endif
#ifdef CONFIG_DAWN_PROG_EXPRESSION
  test_prog_expression,
#endif
#ifdef CONFIG_DAWN_PROG_SELECTOR
  test_prog_selector,
#endif
#ifdef CONFIG_DAWN_PROG_BITPACK
  test_prog_bitpack,
#endif
#ifdef CONFIG_DAWN_PROG_VECPACK
  test_prog_vecpack,
#endif
#ifdef CONFIG_DAWN_PROG_VECSPLIT
  test_prog_vecsplit,
#endif
#ifdef CONFIG_DAWN_PROG_MANYTOONE
  test_prog_manytoone,
#endif
#ifdef CONFIG_DAWN_PROG_ONETOMANY
  test_prog_onetomany,
#endif
#ifdef CONFIG_DAWN_PROG_IOMUX
  test_prog_iomux,
#endif
#ifdef CONFIG_DAWN_PROG_IODEMUX
  test_prog_iodemux,
#endif
#ifdef CONFIG_DAWN_PROG_SEQUENCER
  test_prog_sequencer,
#endif

  nullptr,
};

extern "C"
{
  int test_run_prog()
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
