# boards/arm/stm32f0l0g0/nucleo-c092rc/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../common
                 ${CMAKE_BINARY_DIR}/boards/common)

add_subdirectory(src)
