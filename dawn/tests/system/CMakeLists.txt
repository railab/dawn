# dawn/tests/system/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_SYSTEM_LTE)
  set(SRCS
      test_run.cxx
      test_lte.cxx)

  target_sources(dawn_tests PRIVATE ${SRCS})
endif()
