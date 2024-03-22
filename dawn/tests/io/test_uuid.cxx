// dawn/tests/io/test_uuid.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/uuid.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: UUID IO configuration
//***************************************************************************

static uint32_t g_cfg_uuid[] = {
  CIOUuid::objectId(0),
  0,
};

//***************************************************************************
// Description: UUID IO reports read-only / non-batch / non-notify
// capabilities and a data size matching the board's unique-id length.
//***************************************************************************

static void test_io_uuid_properties()
{
  CDescObject desc(g_cfg_uuid);
  CIOUuid uuid(desc);
  io_sdata_t<uint8_t, CONFIG_BOARDCTL_UNIQUEID_SIZE> uuid_data;

  TEST_ASSERT_EQUAL(OK, uuid.configure());
  TEST_ASSERT_EQUAL(OK, uuid.init());

  TEST_ASSERT_EQUAL(uuid_data.getDataSize(), uuid.getDataSize());
  TEST_ASSERT_EQUAL(CONFIG_BOARDCTL_UNIQUEID_SIZE, uuid.getDataDim());
  TEST_ASSERT_TRUE(uuid.isRead());
  TEST_ASSERT_FALSE(uuid.isWrite());
  TEST_ASSERT_FALSE(uuid.isNotify());
  TEST_ASSERT_FALSE(uuid.isBatch());
}

//***************************************************************************
// Description: UUID IO setData and batched getData both return -ENOTSUP.
//***************************************************************************

static void test_io_uuid_unsupported_ops()
{
  CDescObject desc(g_cfg_uuid);
  CIOUuid uuid(desc);
  io_sdata_t<uint8_t, CONFIG_BOARDCTL_UNIQUEID_SIZE> uuid_data;

  TEST_ASSERT_EQUAL(OK, uuid.configure());
  TEST_ASSERT_EQUAL(OK, uuid.init());

  TEST_ASSERT_EQUAL(-ENOTSUP, uuid.setData(uuid_data));
  TEST_ASSERT_EQUAL(-ENOTSUP, uuid.getData(uuid_data, 2));
}

//***************************************************************************
// Description: UUID IO returns a non-zero unique id from the board.
//***************************************************************************

static void test_io_uuid_read_nonzero()
{
  CDescObject desc(g_cfg_uuid);
  CIOUuid uuid(desc);
  io_sdata_t<uint8_t, CONFIG_BOARDCTL_UNIQUEID_SIZE> uuid_data;
  bool all_zeros = true;
  size_t i;

  TEST_ASSERT_EQUAL(OK, uuid.configure());
  TEST_ASSERT_EQUAL(OK, uuid.init());

  TEST_ASSERT_EQUAL(OK, uuid.getData(uuid_data, 1));

  for (i = 0; i < CONFIG_BOARDCTL_UNIQUEID_SIZE; i++)
    {
      if (uuid_data(i) != 0)
        {
          all_zeros = false;
          break;
        }
    }

  TEST_ASSERT_FALSE(all_zeros);
}

//***************************************************************************
// Description: UUID IO returns the same value across consecutive reads
// (the unique id is constant).
//***************************************************************************

static void test_io_uuid_read_consistent()
{
  CDescObject desc(g_cfg_uuid);
  CIOUuid uuid(desc);
  io_sdata_t<uint8_t, CONFIG_BOARDCTL_UNIQUEID_SIZE> data1;
  io_sdata_t<uint8_t, CONFIG_BOARDCTL_UNIQUEID_SIZE> data2;
  size_t i;

  TEST_ASSERT_EQUAL(OK, uuid.configure());
  TEST_ASSERT_EQUAL(OK, uuid.init());

  TEST_ASSERT_EQUAL(OK, uuid.getData(data1, 1));
  TEST_ASSERT_EQUAL(OK, uuid.getData(data2, 1));

  for (i = 0; i < CONFIG_BOARDCTL_UNIQUEID_SIZE; i++)
    {
      TEST_ASSERT_EQUAL(data1(i), data2(i));
    }
}

extern "C"
{
  int test_io_system_uuid()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_uuid_properties);
    DAWN_RUN_TEST(test_io_uuid_unsupported_ops);
    DAWN_RUN_TEST(test_io_uuid_read_nonzero);
    DAWN_RUN_TEST(test_io_uuid_read_consistent);

    return UNITY_END();
  }
}
