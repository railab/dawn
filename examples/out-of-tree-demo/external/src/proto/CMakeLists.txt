# examples/out-of-tree-demo/external/src/proto/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_OOT_DEMO_PROTO)
  target_sources(dawn PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/my_proto_dummy.cxx"
    "${CMAKE_CURRENT_LIST_DIR}/my_proto_factory.cxx"
  )
  target_include_directories(dawn PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
endif()
