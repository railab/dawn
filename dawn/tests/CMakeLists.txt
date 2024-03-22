# dawn/tests/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_TESTS)
  nuttx_add_library(dawn_tests)

  nuttx_add_subdirectory()

  target_include_directories(dawn_tests PRIVATE include .)
  target_link_libraries(dawn_tests PUBLIC dawn)

  set(CSRCS test_dawn.cxx test_registry.cxx)

  target_sources(dawn_tests PRIVATE ${CSRCS})

  nuttx_add_application(
    NAME
    dawntest
    SRCS
    dawntest_main.cxx
    STACKSIZE
    ${CONFIG_DAWN_TESTS_STACKSIZE}
    DEPENDS
    dawn_tests)

  target_link_libraries(apps_dawntest PRIVATE dawn)

endif()
