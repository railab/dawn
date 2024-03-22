# examples/out-of-tree-demo/boards/sim/sim/sim/src/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BOARD_LATE_INITIALIZE)
  list(APPEND SRCS sim_boot.c)
endif()

list(APPEND SRCS sim_bringup.c)

target_sources(board PRIVATE ${SRCS})

if(CONFIG_DAWN_FAKE_DRIVERS)
  target_link_libraries(board PRIVATE board_common)
endif()
