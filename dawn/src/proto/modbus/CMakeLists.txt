# dawn/src/proto/modbus/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_PROTO_MODBUS)

  set(SRCS regs.cxx)

  if(CONFIG_DAWN_PROTO_MODBUS_RTU)
    list(APPEND SRCS rtu.cxx)
  endif()

  if(CONFIG_DAWN_PROTO_MODBUS_TCP)
    list(APPEND SRCS tcp.cxx)
  endif()

  if(CONFIG_DAWN_PROTO_MODBUS_ASCII)
    list(APPEND SRCS ascii.cxx)
  endif()

  target_sources(dawn PRIVATE ${SRCS})

endif()
