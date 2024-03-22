# examples/out-of-tree-demo/boards/sim/sim/sim/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Out-of-tree sim board for the Dawn OOT demo. The board reuses the
# upstream `boards/common/` library for fake drivers (the path is exposed
# via the DAWN_BOARDS_COMMON env var that dawnpy sets before invoking cmake).

add_subdirectory("$ENV{DAWN_BOARDS_COMMON}"
                 "${CMAKE_BINARY_DIR}/boards/common")

add_subdirectory(src)
