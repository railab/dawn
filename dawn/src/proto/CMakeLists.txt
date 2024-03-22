# dawn/src/proto/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

nuttx_add_subdirectory()

set(SRCS common.cxx simplebase.cxx factory.cxx handler.cxx)

if(CONFIG_DAWN_PROTO_DUMMY)
  list(APPEND SRCS dummy.cxx)
endif()

target_sources(dawn PRIVATE ${SRCS})
