# dawn/src/common/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

nuttx_add_subdirectory()

set(SRCS descriptor.cxx descobject.cxx objectid.cxx objectcfg.cxx object.cxx
         thread.cxx bindable.cxx)

target_sources(dawn PRIVATE ${SRCS})
