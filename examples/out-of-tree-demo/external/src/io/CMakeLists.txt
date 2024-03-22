# examples/out-of-tree-demo/external/src/io/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# User IO classes link into the `dawn` library. Factory registration is
# attached to the application target via factories.cmake (the macros in
# <dawn/oot.hxx> need to live in the executable so the strong overrides
# beat the weak fallbacks in dawn_main.cxx).

if(CONFIG_DAWN_OOT_DEMO_IO)
  target_sources(dawn PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/my_io_dummy.cxx"
    "${CMAKE_CURRENT_LIST_DIR}/my_io_factory.cxx"
  )
  target_include_directories(dawn PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
endif()
