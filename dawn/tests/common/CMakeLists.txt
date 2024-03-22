# dawn/tests/common/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

set(SRCS
    test_bindable.cxx
    test_threaded.cxx
    test_handler.cxx
    test_descriptor.cxx
    test_descswitch.cxx
    test_descobject.cxx
    test_objectid.cxx
    test_objectcfg.cxx
    test_object.cxx
    test_run.cxx)

target_sources(dawn_tests PRIVATE ${SRCS})
