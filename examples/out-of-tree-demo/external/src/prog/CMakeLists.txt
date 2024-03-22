# examples/out-of-tree-demo/external/src/prog/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_OOT_DEMO_PROG)
  target_sources(dawn PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/my_prog_dummy.cxx"
    "${CMAKE_CURRENT_LIST_DIR}/my_prog_factory.cxx"
  )
  target_include_directories(dawn PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
endif()
