# boards/arm/nrf53/nrf5340-dk/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

# Pull in dawn fake-driver helpers (boards/common).

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../common
                 ${CMAKE_BINARY_DIR}/boards/common)

add_subdirectory(src)
