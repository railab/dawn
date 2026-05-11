# dawn/apps/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

nuttx_add_subdirectory()

get_filename_component(_dawn_apps_real "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)
get_filename_component(_dawn_root "${_dawn_apps_real}/../.." REALPATH)
set(_dawn_examples_apps_dir "${_dawn_root}/examples/apps")
if(EXISTS "${_dawn_examples_apps_dir}/CMakeLists.txt")
  add_subdirectory(
    "${_dawn_examples_apps_dir}"
    "${CMAKE_CURRENT_BINARY_DIR}/examples_apps"
  )
endif()

# Out-of-tree user apps (no-op for in-tree builds).

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dawn_oot.cmake")
dawn_oot_link_apps()
