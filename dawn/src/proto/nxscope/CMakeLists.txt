# dawn/src/proto/nxscope/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_PROTO_NXSCOPE)
  set(SRCS nxscope.cxx)

  if(CONFIG_DAWN_PROTO_NXSCOPE_DUMMY)
    list(APPEND SRCS dummy.cxx)
  endif()

  if(CONFIG_DAWN_PROTO_NXSCOPE_SERIAL)
    list(APPEND SRCS serial.cxx)
  endif()

  if(CONFIG_DAWN_PROTO_NXSCOPE_UDP)
    list(APPEND SRCS udp.cxx)
  endif()

  target_sources(dawn PRIVATE ${SRCS})
endif()
