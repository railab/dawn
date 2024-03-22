// dawn/tests/common/test_bindable.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/bindable.hxx"
#include "test_common.hxx"

#include "test_iomockcommon.hxx"

using namespace dawn;

static constexpr auto BINDABLE_OBJ_ID =
  SObjectId::objectId(SObjectId::OBJTYPE_PROG, 1, SObjectId::DTYPE_ANY, 0, 0);
static constexpr auto IO_BIND_ID_1 = (CIOMockCommon::objectId(0));
static constexpr auto IO_BIND_ID_2 = (CIOMockCommon::objectId(2));
static constexpr auto NON_IO_ID =
  SObjectId::objectId(SObjectId::OBJTYPE_PROG, 2, SObjectId::DTYPE_ANY, 0, 0);

class CBindableObjectMock : public CBindableObject
{
public:
  explicit CBindableObjectMock(CDescObject &desc)
    : CBindableObject(desc)
  {
  }
};

static void test_common_bindable_iomap()
{
  uint32_t bindable_cfg[] = {BINDABLE_OBJ_ID, 0};
  uint32_t io_cfg1[] = {IO_BIND_ID_1, 0};
  uint32_t io_cfg2[] = {IO_BIND_ID_2, 0};
  CDescObject bindable_desc(bindable_cfg);
  CBindableObjectMock bindable(bindable_desc);
  CDescObject io_desc1(io_cfg1);
  CDescObject io_desc2(io_cfg2);
  CIOMockCommon io1(io_desc1);
  CIOMockCommon io2(io_desc2);

  const std::map<SObjectId::ObjectId, CIOCommon *> *iomap = &bindable.getIOMap();

  TEST_ASSERT_FALSE(iomap->contains(IO_BIND_ID_1));
  TEST_ASSERT_NULL(bindable.getIO(IO_BIND_ID_1));
  TEST_ASSERT_NULL(bindable.getObject(IO_BIND_ID_1));

  bindable.setObjectMapItem(IO_BIND_ID_1, nullptr);

  TEST_ASSERT_TRUE(iomap->contains(IO_BIND_ID_1));
  TEST_ASSERT_NULL(iomap->at(IO_BIND_ID_1));
  TEST_ASSERT_NULL(bindable.getIO(IO_BIND_ID_1));
  TEST_ASSERT_NULL(bindable.getObject(IO_BIND_ID_1));

  bindable.setObjectMapItem(IO_BIND_ID_1, &io1);
  TEST_ASSERT_EQUAL(&io1, bindable.getIO(IO_BIND_ID_1));
  TEST_ASSERT_EQUAL(&io1, bindable.getObject(IO_BIND_ID_1));

  // insert_or_assign updates an existing mapping
  bindable.setObjectMapItem(IO_BIND_ID_1, &io2);
  TEST_ASSERT_EQUAL(&io2, bindable.getIO(IO_BIND_ID_1));
}

static void test_common_bindable_unsupported_type()
{
  uint32_t bindable_cfg[] = {BINDABLE_OBJ_ID, 0};
  uint32_t io_cfg1[] = {IO_BIND_ID_1, 0};
  CDescObject bindable_desc(bindable_cfg);
  CBindableObjectMock bindable(bindable_desc);
  CDescObject io_desc1(io_cfg1);
  CIOMockCommon io1(io_desc1);

  const std::map<SObjectId::ObjectId, CIOCommon *> *iomap = &bindable.getIOMap();

  TEST_ASSERT_EQUAL(0, static_cast<int>(iomap->size()));

  bindable.setObjectMapItem(NON_IO_ID, &io1);

  TEST_ASSERT_EQUAL(0, static_cast<int>(iomap->size()));
  TEST_ASSERT_NULL(bindable.getObject(NON_IO_ID));
}

extern "C"
{
  int test_common_bindable()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_common_bindable_iomap);
    DAWN_RUN_TEST(test_common_bindable_unsupported_type);

    return UNITY_END();
  }
}
