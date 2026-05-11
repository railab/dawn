# examples/apps/usensor_reader/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_EXAMPLES_USENSOR_READER)
  nuttx_add_application(
    NAME
    usensor_reader
    SRCS
    usensor_reader_main.c
    STACKSIZE
    ${CONFIG_EXAMPLES_USENSOR_READER_STACKSIZE})
endif()
