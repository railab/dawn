# examples/out-of-tree-demo/external/dawn_oot.cmake
#
# SPDX-License-Identifier: Apache-2.0
#

include_guard(GLOBAL)

set(
  DAWN_OOT_DEMO_EXTERNAL_DIR "${CMAKE_CURRENT_LIST_DIR}"
  CACHE INTERNAL "OOT demo external directory"
)

function(dawn_oot_user_link_dawn target)
  add_subdirectory(
    "${DAWN_OOT_DEMO_EXTERNAL_DIR}/src/io"
    "${CMAKE_BINARY_DIR}/oot/src/io"
  )
  add_subdirectory(
    "${DAWN_OOT_DEMO_EXTERNAL_DIR}/src/prog"
    "${CMAKE_BINARY_DIR}/oot/src/prog"
  )
  add_subdirectory(
    "${DAWN_OOT_DEMO_EXTERNAL_DIR}/src/proto"
    "${CMAKE_BINARY_DIR}/oot/src/proto"
  )
endfunction()


function(dawn_oot_user_link_apps)
  add_subdirectory(
    "${DAWN_OOT_DEMO_EXTERNAL_DIR}/apps"
    "${CMAKE_BINARY_DIR}/oot/apps"
  )
endfunction()


function(dawn_oot_user_setup_app target)
  target_sources(${target} PRIVATE
    "${DAWN_OOT_DEMO_EXTERNAL_DIR}/dawn_oot_hooks.cxx"
  )
endfunction()
