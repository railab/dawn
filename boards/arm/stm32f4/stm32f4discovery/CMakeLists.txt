# boards/arm/stm32f4/stm32f4discovery/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../common
                 ${CMAKE_BINARY_DIR}/boards/common)

add_subdirectory(src)
