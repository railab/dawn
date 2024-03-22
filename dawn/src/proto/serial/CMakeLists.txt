# dawn/src/proto/serial/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_PROTO_SERIAL)
  set(SRCS)

  list(APPEND SRCS simple.cxx)

  target_sources(dawn PRIVATE ${SRCS})
endif()
