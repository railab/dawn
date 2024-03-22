#
# boards/x86_64/qemu/qemu-intel64/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../common
                 ${CMAKE_BINARY_DIR}/boards/common)

add_subdirectory(src)
