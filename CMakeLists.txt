# CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

# NuttX apps entry point for the Dawn repository.
#
# external/apps/external is expected to point at the repository root. Keep
# component registration here so dawn/ does not depend on sibling examples/.

add_subdirectory(dawn)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/apps/CMakeLists.txt")
  add_subdirectory(examples/apps)
endif()

nuttx_generate_kconfig(
  MENUDESC "Dawn Repository"
  EXTERNAL_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/examples/apps"
)
