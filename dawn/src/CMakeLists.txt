# dawn/src/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

nuttx_add_subdirectory()

set(SRCS dawn.cxx oot.cxx)

target_sources(dawn PRIVATE ${SRCS})

# Out-of-tree user extensions (no-op for in-tree builds).

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dawn_oot.cmake")
dawn_oot_link()

nuttx_generate_kconfig(MENUDESC "Dawn Framework")
