// dawn/tests/io/test_uname.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/uname.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: hostname IO
//***************************************************************************

static uint32_t g_cfg_uname_hostname[] = {
  CIOUname::objectIdHostname(),
  0,
};

//***************************************************************************
// Description: hostname IO reports a data size of HOST_NAME_MAX bytes.
//***************************************************************************

static void test_io_system_hostname_size()
{
  CDescObject desc(g_cfg_uname_hostname);
  CIOUname uname(desc);

  TEST_ASSERT_EQUAL(OK, uname.configure());
  TEST_ASSERT_EQUAL(OK, uname.init());

  TEST_ASSERT_EQUAL(HOST_NAME_MAX, uname.getDataSize());
}

//***************************************************************************
// Description: hostname IO setData and batched getData both return
// -ENOTSUP.
//***************************************************************************

static void test_io_system_hostname_unsupported_ops()
{
  CDescObject desc(g_cfg_uname_hostname);
  CIOUname uname(desc);
  io_sdata_t<char, HOST_NAME_MAX> hostname;

  TEST_ASSERT_EQUAL(OK, uname.configure());
  TEST_ASSERT_EQUAL(OK, uname.init());

  TEST_ASSERT_EQUAL(-ENOTSUP, uname.setData(hostname));
  TEST_ASSERT_EQUAL(-ENOTSUP, uname.getData(hostname, 2));
}

//***************************************************************************
// Description: hostname IO reads back the configured CONFIG_LIBC_HOSTNAME
// string.
//***************************************************************************

static void test_io_system_hostname_value()
{
  CDescObject desc(g_cfg_uname_hostname);
  CIOUname uname(desc);
  io_sdata_t<char, HOST_NAME_MAX> hostname;
  const char *expected = CONFIG_LIBC_HOSTNAME;
  size_t i;

  TEST_ASSERT_EQUAL(OK, uname.configure());
  TEST_ASSERT_EQUAL(OK, uname.init());

  TEST_ASSERT_EQUAL(OK, uname.getData(hostname, 1));

  for (i = 0; i < 9; i++)
    {
      TEST_ASSERT_EQUAL(expected[i], hostname(i));
    }
}

extern "C"
{
  int test_io_system_uname()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_system_hostname_size);
    DAWN_RUN_TEST(test_io_system_hostname_unsupported_ops);
    DAWN_RUN_TEST(test_io_system_hostname_value);

    return UNITY_END();
  }
}
