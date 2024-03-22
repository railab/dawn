# dawn/tests/porting/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

set(SRCS test_run.cxx test_nuttx.cxx)

target_sources(dawn_tests PRIVATE ${SRCS})
