// dawn/tests/common/test_handler.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/handler.hxx"
#include "test_common.hxx"

using namespace dawn;

static void test_handler_init()
{
  // Nothing here now
}

extern "C"
{
  int test_common_handler()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_handler_init);

    return UNITY_END();
  }
}
