# dawn/src/proto/can/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_PROTO_CAN)
  set(SRCS regs.cxx can.cxx)

  if(CONFIG_DAWN_PROTO_CAN_SEG OR CONFIG_DAWN_PROTO_CAN_SINGLE_ID)
    list(APPEND SRCS isotp.cxx)
  endif()

  target_sources(dawn PRIVATE ${SRCS})
endif()
