# dawn/apps/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

nuttx_add_subdirectory()

# Out-of-tree user apps (no-op for in-tree builds).

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dawn_oot.cmake")
dawn_oot_link_apps()
