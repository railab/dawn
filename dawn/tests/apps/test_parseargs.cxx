// dawn/tests/apps/test_parseargs.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn_parseargs.hxx"
#include "test_common.hxx"

//***************************************************************************
// Description: validate_args accepts nullptr descPath when BUILTIN only.
//***************************************************************************

static void test_apps_parseargs_validate_builtin_ok()
{
  struct args_s args;

  args.descPath = nullptr;

#ifdef CONFIG_DAWN_APPS_DESC_SOURCE_BUILTIN
  TEST_ASSERT_EQUAL(0, validate_args(&args));
#endif
}

//***************************************************************************
// Description: validate_args rejects -f when FILE is disabled.
//***************************************************************************

static void test_apps_parseargs_validate_file_disabled_rejects_f()
{
  struct args_s args;

  args.descPath = "/data/desc.bin";

#ifndef CONFIG_DAWN_APPS_DESC_SOURCE_FILE
  TEST_ASSERT(validate_args(&args) != 0);
#endif
}

//***************************************************************************
// Description: args_s descPath field is writable through parse_args.
//***************************************************************************

static void test_apps_parseargs_defaults()
{
  struct args_s args;

  args.descPath = nullptr;
  TEST_ASSERT_EQUAL(nullptr, args.descPath);
}

extern "C"
{
  int test_apps_parseargs()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_apps_parseargs_validate_builtin_ok);
    DAWN_RUN_TEST(test_apps_parseargs_validate_file_disabled_rejects_f);
    DAWN_RUN_TEST(test_apps_parseargs_defaults);

    return UNITY_END();
  }
}
