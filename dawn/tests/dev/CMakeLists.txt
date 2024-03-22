# dawn/tests/dev/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

set(SRCS
    test_run.cxx
    test_descriptor.cxx)

if(CONFIG_DAWN_INSPECT)
  list(APPEND SRCS test_inspector.cxx)
endif()

target_sources(dawn_tests PRIVATE ${SRCS})
