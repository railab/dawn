// dawn/tests/io/test_common.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "test_common.hxx"

#include "dawn/io/common.hxx"
#include "dawn/io/sdata.hxx"
#include "test_iomockcommon.hxx"

using namespace dawn;

static constexpr auto CONFIGID_1 = CIOCommon::cfgIdDevno(true);
static constexpr auto CONFIGID_2 = CIOCommon::cfgIdLimitMin(SObjectId::DTYPE_UINT32, 1);
static constexpr auto CONFIGID_3 = CIOCommon::cfgIdLimitMax(SObjectId::DTYPE_UINT32, 1);
static constexpr auto CONFIGID_4 = CIOCommon::cfgIdLimitStep(SObjectId::DTYPE_UINT32, 1);

//***************************************************************************
// Description: simple IO
//***************************************************************************

static uint32_t g_cfg_valid_io1[] = {
  // ObjectID

  SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 0),

  // CFG Size

  0};

//***************************************************************************
// Description: simple IO with devno
//***************************************************************************

static uint32_t g_cfg_valid_io2[] = {
  // ObjectID

  SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 0),

  // CFG Size

  1,

  CONFIGID_1,
  3,
};

//***************************************************************************
// Description: simple IO with limits
//***************************************************************************

static uint32_t g_cfg_valid_io3[] = {
  // ObjectID

  SObjectId::objectId(SObjectId::OBJTYPE_IO, 1, 1, 0, 0),

  // CFG Size

  3,

  CONFIGID_2,
  0,
  CONFIGID_3,
  100,
  CONFIGID_4,
  1,
};

//***************************************************************************
// Description: common IO defaults expose basic capabilities without config.
//***************************************************************************

static void test_io_common_init()
{
  CDescObject desc(g_cfg_valid_io1);
  CIOMockCommon mock(desc);
  CIOCommon *cmn = &mock;
  io_sdata_t<uint32_t, 1> tmp;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, cmn->init());

  // Tests

  TEST_ASSERT(cmn->getData(tmp, 1) == OK);
  TEST_ASSERT(cmn->getData(tmp, 2) == OK);
  TEST_ASSERT(cmn->isRead() == true);
  TEST_ASSERT(cmn->isWrite() == true);
  TEST_ASSERT(cmn->isNotify() == false);
  TEST_ASSERT(cmn->isTimestamp() == false);
  TEST_ASSERT(cmn->getFd() == -ENOTSUP);

  TEST_ASSERT_EQUAL(mock.getDevno(), 0);
  TEST_ASSERT(mock.getLimitMin() == nullptr);
  TEST_ASSERT(mock.getLimitMax() == nullptr);
  TEST_ASSERT(mock.getLimitStep() == nullptr);
  TEST_ASSERT_EQUAL(0, mock.getLimitWords());
}

//***************************************************************************
// Description: common IO reads the configured device number.
//***************************************************************************

static void test_io_common_devno()
{
  CDescObject desc(g_cfg_valid_io2);
  CIOMockCommon mock(desc);
  CIOCommon *cmn = &mock;
  io_sdata_t<uint32_t, 1> tmp;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, cmn->init());

  // Tests

  TEST_ASSERT(cmn->getData(tmp, 1) == OK);
  TEST_ASSERT(cmn->getData(tmp, 2) == OK);
  TEST_ASSERT(cmn->isRead() == true);
  TEST_ASSERT(cmn->isWrite() == true);
  TEST_ASSERT(cmn->isNotify() == false);
  TEST_ASSERT(cmn->isTimestamp() == false);
  TEST_ASSERT(cmn->getFd() == -ENOTSUP);

  TEST_ASSERT(mock.getDevno() == 3);
  TEST_ASSERT(mock.getLimitMin() == nullptr);
  TEST_ASSERT(mock.getLimitMax() == nullptr);
  TEST_ASSERT(mock.getLimitStep() == nullptr);
  TEST_ASSERT_EQUAL(0, mock.getLimitWords());
}

//***************************************************************************
// Description: common IO exposes configured min/max/step limits.
//***************************************************************************

static void test_io_common_limit()
{
  CDescObject desc(g_cfg_valid_io3);
  CIOMockCommon mock(desc);
  CIOCommon *cmn = &mock;
  io_sdata_t<uint32_t, 1> tmp;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, cmn->init());

  // Tests

  TEST_ASSERT(cmn->getData(tmp, 1) == OK);
  TEST_ASSERT(cmn->getData(tmp, 2) == OK);
  TEST_ASSERT(cmn->isRead() == true);
  TEST_ASSERT(cmn->isWrite() == true);
  TEST_ASSERT(cmn->isNotify() == false);
  TEST_ASSERT(cmn->isTimestamp() == false);
  TEST_ASSERT(cmn->getFd() == -ENOTSUP);

  TEST_ASSERT_EQUAL(mock.getDevno(), 0);
  TEST_ASSERT(mock.getLimitMin() != nullptr);
  TEST_ASSERT(mock.getLimitMax() != nullptr);
  TEST_ASSERT(mock.getLimitStep() != nullptr);
  TEST_ASSERT_EQUAL(1, mock.getLimitWords());
  TEST_ASSERT_EQUAL(0u, mock.getLimitMin()[0]);
  TEST_ASSERT_EQUAL(100u, mock.getLimitMax()[0]);
  TEST_ASSERT_EQUAL(1u, mock.getLimitStep()[0]);
}

//***************************************************************************
// Description: common IO config get/set enforces access and size rules.
//***************************************************************************

static void test_io_common_cfgobj()
{
  CDescObject desc1(g_cfg_valid_io1);
  CIOMockCommon mock1(desc1);
  CIOCommon *cmn1 = &mock1;
  CDescObject desc2(g_cfg_valid_io2);
  CIOMockCommon mock2(desc2);
  CIOCommon *cmn2 = &mock2;
  CDescObject desc3(g_cfg_valid_io3);
  CIOMockCommon mock3(desc3);
  CIOCommon *cmn3 = &mock3;
  uint32_t data = 0;
  uint32_t data2[4];
  int ret;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, cmn1->init());
  TEST_ASSERT_EQUAL(OK, cmn2->init());
  TEST_ASSERT_EQUAL(OK, cmn3->init());

  // Get not vailalbe config

  ret = cmn1->getConfig(CONFIGID_1, &data, 1);
  TEST_ASSERT_EQUAL(ret, -EINVAL);
  TEST_ASSERT_EQUAL(data, 0);

  data = 0;
  ret = cmn1->getConfig(CONFIGID_2, &data, 1);
  TEST_ASSERT_EQUAL(ret, -EINVAL);
  TEST_ASSERT_EQUAL(data, 0);

  // Set not availalbe config

  data = 2;
  ret = cmn1->setConfig(CONFIGID_1, &data, 1);
  TEST_ASSERT_EQUAL(ret, -EINVAL);

  data = 2;
  ret = cmn1->setConfig(CONFIGID_2, &data, 1);
  TEST_ASSERT_EQUAL(ret, -EACCES);

  // Get availalbe config

  data = 0;
  ret = cmn2->getConfig(CONFIGID_1, &data, 1);
  TEST_ASSERT_EQUAL(ret, OK);
  TEST_ASSERT_EQUAL(data, 3);

  // Set RW config

  data = 2;
  ret = cmn2->setConfig(CONFIGID_1, &data, 1);
  TEST_ASSERT_EQUAL(ret, OK);

  data = 0;
  ret = cmn2->getConfig(CONFIGID_1, &data, 1);
  TEST_ASSERT_EQUAL(ret, OK);
  TEST_ASSERT_EQUAL(data, 2);

  // Set RO config

  data2[0] = 2;
  data2[1] = 2;
  data2[2] = 2;
  data2[3] = 0;
  ret = cmn3->setConfig(CONFIGID_2, data2, 3);
  TEST_ASSERT_EQUAL(ret, -EACCES);

  ret = cmn3->getConfig(CONFIGID_2, data2, 1);
  TEST_ASSERT_EQUAL(ret, OK);
  TEST_ASSERT_EQUAL(data2[0], 0);

  ret = cmn3->getConfig(CONFIGID_3, data2, 1);
  TEST_ASSERT_EQUAL(ret, OK);
  TEST_ASSERT_EQUAL(data2[0], 100);

  ret = cmn3->getConfig(CONFIGID_4, data2, 1);
  TEST_ASSERT_EQUAL(ret, OK);
  TEST_ASSERT_EQUAL(data2[0], 1);
}

extern "C"
{
  int test_io_common()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_common_init);
    DAWN_RUN_TEST(test_io_common_devno);
    DAWN_RUN_TEST(test_io_common_limit);
    DAWN_RUN_TEST(test_io_common_cfgobj);

    return UNITY_END();
  }
}
