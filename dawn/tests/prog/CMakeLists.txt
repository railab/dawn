# dawn/tests/prog/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

set(SRCS test_run.cxx test_factory.cxx test_common.cxx test_handler.cxx
         test_process_common.cxx)

if(CONFIG_DAWN_PROG_ADJUST)
  list(APPEND SRCS test_adjust.cxx)
endif()

if(CONFIG_DAWN_PROG_GATEWAY)
  list(APPEND SRCS test_gateway.cxx)
endif()

if(CONFIG_DAWN_PROG_LATEST)
  list(APPEND SRCS test_latest.cxx)
endif()

if(CONFIG_DAWN_PROG_REDIRECT)
  list(APPEND SRCS test_redirect.cxx)
endif()

if(CONFIG_DAWN_PROG_BUFFER)
  list(APPEND SRCS test_buffer.cxx)
endif()

if(CONFIG_DAWN_PROG_MOVING_AVG)
  list(APPEND SRCS test_moving_avg.cxx)
endif()

if(CONFIG_DAWN_PROG_IIR_FILTER)
  list(APPEND SRCS test_iir_filter.cxx)
endif()

if(CONFIG_DAWN_PROG_THRESHOLD_ANY)
  list(APPEND SRCS test_threshold.cxx)
endif()

if(CONFIG_DAWN_PROG_PROCESS)
  list(APPEND SRCS test_process.cxx)
endif()

if(CONFIG_DAWN_PROG_STATS_MIN)
  list(APPEND SRCS test_stats_min.cxx)
endif()

if(CONFIG_DAWN_PROG_STATS_MAX)
  list(APPEND SRCS test_stats_max.cxx)
endif()

if(CONFIG_DAWN_PROG_STATS_AVG)
  list(APPEND SRCS test_stats_avg.cxx)
endif()

if(CONFIG_DAWN_PROG_STATS_SUM)
  list(APPEND SRCS test_stats_sum.cxx)
endif()

if(CONFIG_DAWN_PROG_STATS_COUNT)
  list(APPEND SRCS test_stats_count.cxx)
endif()

if(CONFIG_DAWN_PROG_STATS_RMS)
  list(APPEND SRCS test_stats_rms.cxx)
endif()

if(CONFIG_DAWN_PROG_SAMPLING)
  list(APPEND SRCS test_sampling.cxx)
endif()

if(CONFIG_DAWN_PROG_SEQUENCER)
  list(APPEND SRCS test_sequencer.cxx)
endif()

if(CONFIG_DAWN_PROG_BITSPLIT)
  list(APPEND SRCS test_bitsplit.cxx)
endif()

if(CONFIG_DAWN_PROG_TOGGLE)
  list(APPEND SRCS test_toggle.cxx)
endif()

if(CONFIG_DAWN_PROG_COUNTER)
  list(APPEND SRCS test_counter.cxx)
endif()

if(CONFIG_DAWN_PROG_SWITCH)
  list(APPEND SRCS test_switch.cxx)
endif()

if(CONFIG_DAWN_PROG_EXPRESSION)
  list(APPEND SRCS test_expression.cxx)
endif()

if(CONFIG_DAWN_PROG_SELECTOR)
  list(APPEND SRCS test_selector.cxx)
endif()

if(CONFIG_DAWN_PROG_BITPACK)
  list(APPEND SRCS test_bitpack.cxx)
endif()

if(CONFIG_DAWN_PROG_VECPACK)
  list(APPEND SRCS test_vecpack.cxx)
endif()

if(CONFIG_DAWN_PROG_VECSPLIT)
  list(APPEND SRCS test_vecsplit.cxx)
endif()

if(CONFIG_DAWN_PROG_MANYTOONE)
  list(APPEND SRCS test_manytoone.cxx)
endif()

if(CONFIG_DAWN_PROG_ONETOMANY)
  list(APPEND SRCS test_onetomany.cxx)
endif()

if(CONFIG_DAWN_PROG_IOMUX)
  list(APPEND SRCS test_iomux.cxx)
endif()

if(CONFIG_DAWN_PROG_IODEMUX)
  list(APPEND SRCS test_iodemux.cxx)
endif()

target_sources(dawn_tests PRIVATE ${SRCS})
