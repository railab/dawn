// dawn/tests/io/test_factory.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/common.hxx"
#include "dawn/io/factory.hxx"
#include "test_common.hxx"

using namespace dawn;

// Description: helper builds dummy IO descriptor objects for factory tests.
//***************************************************************************

// Build a SObjectCfgData for a dummy IO with the supplied priv index.

static SObjectCfg::SObjectCfgData make_io_dummy_objcfg(uint32_t priv)
{
  SObjectCfg::SObjectCfgData cfg;

  cfg.objid.s.type = SObjectId::OBJTYPE_IO;
  cfg.objid.s.cls = CIOCommon::IO_CLASS_DUMMY;
  cfg.objid.s.dtype = SObjectId::DTYPE_UINT32;
  cfg.objid.s.flags = 1;
  cfg.objid.s.priv = priv;
  cfg.size = 0;
  return cfg;
}

//***************************************************************************
// Description: factory.create() returns a non-null CIOCommon for a valid
// dummy IO descriptor.
//***************************************************************************

static void test_io_factory_creates_dummy()
{
  CIOFactory factory;
  SObjectCfg::SObjectCfgData cfg = make_io_dummy_objcfg(0);
  CDescObject desc(cfg);
  CIOCommon *obj;

  obj = factory.create(desc);
  TEST_ASSERT_NOT_NULL(obj);
  delete obj;
}

//***************************************************************************
// Description: two consecutive create() calls produce independent
// instances (the factory does not return a cached singleton).
//***************************************************************************

static void test_io_factory_creates_distinct_instances()
{
  CIOFactory factory;
  SObjectCfg::SObjectCfgData cfg1 = make_io_dummy_objcfg(0);
  SObjectCfg::SObjectCfgData cfg2 = make_io_dummy_objcfg(0);
  CDescObject desc1(cfg1);
  CDescObject desc2(cfg2);
  CIOCommon *a;
  CIOCommon *b;

  a = factory.create(desc1);
  b = factory.create(desc2);
  TEST_ASSERT_NOT_NULL(a);
  TEST_ASSERT_NOT_NULL(b);
  TEST_ASSERT(a != b);

  delete a;
  delete b;
}

extern "C"
{
  int test_io_factory()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_factory_creates_dummy);
    DAWN_RUN_TEST(test_io_factory_creates_distinct_instances);

    return UNITY_END();
  }
}
