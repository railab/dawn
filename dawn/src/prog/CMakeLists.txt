# dawn/src/prog/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

nuttx_add_subdirectory()

set(SRCS handler.cxx common.cxx factory.cxx)

if(CONFIG_DAWN_PROG_PROCESS)
  list(APPEND SRCS process.cxx)
endif()

if(CONFIG_DAWN_PROG_ADJUST)
  list(APPEND SRCS adjust.cxx)
endif()

if(CONFIG_DAWN_PROG_DUMMY)
  list(APPEND SRCS dummy.cxx)
endif()

if(CONFIG_DAWN_PROG_GATEWAY)
  list(APPEND SRCS gateway.cxx)
endif()

if(CONFIG_DAWN_PROG_REDIRECT)
  list(APPEND SRCS redirect.cxx)
endif()

if(CONFIG_DAWN_PROG_BUFFER)
  list(APPEND SRCS buffer.cxx)
endif()

if(CONFIG_DAWN_PROG_SAMPLING)
  list(APPEND SRCS sampling.cxx)
endif()

if(CONFIG_DAWN_PROG_SEQUENCER)
  list(APPEND SRCS sequencer.cxx)
endif()

if(CONFIG_DAWN_PROG_BITSPLIT)
  list(APPEND SRCS bitsplit.cxx)
endif()

if(CONFIG_DAWN_PROG_TOGGLE)
  list(APPEND SRCS toggle.cxx)
endif()

if(CONFIG_DAWN_PROG_COUNTER)
  list(APPEND SRCS counter.cxx)
endif()

if(CONFIG_DAWN_PROG_SWITCH)
  list(APPEND SRCS switch.cxx)
endif()

if(CONFIG_DAWN_PROG_EXPRESSION)
  list(APPEND SRCS expression.cxx)
endif()

if(CONFIG_DAWN_PROG_SELECTOR)
  list(APPEND SRCS selector.cxx)
endif()

if(CONFIG_DAWN_PROG_BITPACK)
  list(APPEND SRCS bitpack.cxx)
endif()

if(CONFIG_DAWN_PROG_VECPACK)
  list(APPEND SRCS vecpack.cxx)
endif()

if(CONFIG_DAWN_PROG_VECSPLIT)
  list(APPEND SRCS vecsplit.cxx)
endif()

if(CONFIG_DAWN_PROG_MANYTOONE)
  list(APPEND SRCS manytoone.cxx)
endif()

if(CONFIG_DAWN_PROG_ONETOMANY)
  list(APPEND SRCS onetomany.cxx)
endif()

if(CONFIG_DAWN_PROG_IOMUX)
  list(APPEND SRCS iomux.cxx)
endif()

if(CONFIG_DAWN_PROG_IODEMUX)
  list(APPEND SRCS iodemux.cxx)
endif()

target_sources(dawn PRIVATE ${SRCS})
