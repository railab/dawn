# dawn/tests/apps/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

set(SRCS
    test_run.cxx
    test_parseargs.cxx)

if(CONFIG_DAWN_APPS_DESC_SOURCE_FILE)
  list(APPEND SRCS test_file_load.cxx)
  target_sources(dawn_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../apps/dawn/dawn_file_load.cxx)
endif()

target_sources(dawn_tests PRIVATE ${SRCS})
target_include_directories(dawn_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../apps/dawn)
