# dawn/src/dev/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

nuttx_add_subdirectory()

set(SRCS descriptor.cxx shutdown.cxx)

if(CONFIG_DAWN_DESC_SWITCH)
  list(APPEND SRCS descswitch.cxx)
endif()

# Only compile inspector when CONFIG_DAWN_INSPECT is enabled

if(CONFIG_DAWN_INSPECT)
  list(APPEND SRCS inspector.cxx)
endif()

target_sources(dawn PRIVATE ${SRCS})
