# dawn/src/system/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

nuttx_add_subdirectory()

set(SRCS handler.cxx factory.cxx)

if(CONFIG_DAWN_SYSTEM_LTE)
  list(APPEND SRCS lte.cxx)
endif()

if(CONFIG_DAWN_SYSTEM_GNSS)
  list(APPEND SRCS gnss.cxx)
endif()

target_sources(dawn PRIVATE ${SRCS})
