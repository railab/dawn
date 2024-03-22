# dawn/src/proto/shell/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_PROTO_SHELL)
  set(SRCS)

  if(CONFIG_DAWN_PROTO_SHELL_PRETTY)
    list(APPEND SRCS pretty.cxx)
  endif()

  if(CONFIG_DAWN_PROTO_SHELL_INSPECT)
    list(APPEND SRCS inspect.cxx)
  endif()

  target_sources(dawn PRIVATE ${SRCS})
endif()
