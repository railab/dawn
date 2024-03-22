// dawn/tests/prog/test_factory.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/common.hxx"
#include "dawn/prog/dummy.hxx"
#include "dawn/prog/factory.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: program factory creates dummy programs for valid descriptors.
//***************************************************************************

static void test_prog_factory_run()
{
#ifdef CONFIG_DAWN_PROG_DUMMY
  CProgFactory factory;
  SObjectCfg::SObjectCfgData objcfg2;
  SObjectCfg::SObjectCfgData objcfg1;
  CProgCommon *tmp1 = nullptr;
  CProgCommon *tmp2 = nullptr;

  /* Valid object */

  objcfg1.objid.s.type = SObjectId::OBJTYPE_PROG;
  objcfg1.objid.s.cls = CProgCommon::PROG_CLASS_DUMMY;
  objcfg1.objid.s.dtype = 1;
  objcfg1.objid.s.flags = 0;
  objcfg1.objid.s.priv = 0;
  objcfg1.size = 0;

  CDescObject desc1(objcfg1);
  tmp1 = factory.create(desc1);
  TEST_ASSERT(tmp1 != nullptr);

  objcfg2.objid.s.type = SObjectId::OBJTYPE_PROG;
  objcfg2.objid.s.cls = CProgCommon::PROG_CLASS_DUMMY;
  objcfg2.objid.s.dtype = 1;
  objcfg2.objid.s.flags = 0;
  objcfg2.objid.s.priv = 2;
  objcfg2.size = 0;

  CDescObject desc2(objcfg2);
  tmp2 = factory.create(desc2);
  TEST_ASSERT(tmp2 != nullptr);

  delete tmp1;
  delete tmp2;
#else
  TEST_IGNORE_MESSAGE("dummy prog not enabled");
#endif
}

extern "C"
{
  int test_prog_factory()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_prog_factory_run);

    return UNITY_END();
  }
}
