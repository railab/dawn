// dawn/tests/proto/test_factory.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/dummy.hxx"
#include "dawn/proto/factory.hxx"
#include "test_common.hxx"

using namespace dawn;

#ifdef CONFIG_DAWN_PROTO_DUMMY

// Build a SObjectCfgData for a dummy proto with the supplied priv index.

static SObjectCfg::SObjectCfgData make_dummy_objcfg(uint32_t priv)
{
  SObjectCfg::SObjectCfgData cfg;

  cfg.objid.s.type = SObjectId::OBJTYPE_PROTO;
  cfg.objid.s.cls = CProtoCommon::PROTO_CLASS_DUMMY;
  cfg.objid.s.dtype = 1;
  cfg.objid.s.flags = 0;
  cfg.objid.s.priv = priv;
  cfg.size = 0;
  return cfg;
}

//***************************************************************************
// Description: factory.create() returns a non-null CProtoCommon for a
// dummy proto descriptor.
//***************************************************************************

static void test_proto_factory_creates_dummy()
{
  CProtoFactory factory;
  SObjectCfg::SObjectCfgData cfg = make_dummy_objcfg(0);
  CDescObject desc(cfg);
  CProtoCommon *obj;

  obj = factory.create(desc);
  TEST_ASSERT_NOT_NULL(obj);
  delete obj;
}

//***************************************************************************
// Description: two distinct priv ids yield two independent CProtoCommon
// instances from the same factory.
//***************************************************************************

static void test_proto_factory_creates_distinct_instances()
{
  CProtoFactory factory;
  SObjectCfg::SObjectCfgData cfg1 = make_dummy_objcfg(0);
  SObjectCfg::SObjectCfgData cfg2 = make_dummy_objcfg(2);
  CDescObject desc1(cfg1);
  CDescObject desc2(cfg2);
  CProtoCommon *a;
  CProtoCommon *b;

  a = factory.create(desc1);
  b = factory.create(desc2);
  TEST_ASSERT_NOT_NULL(a);
  TEST_ASSERT_NOT_NULL(b);
  TEST_ASSERT(a != b);

  delete a;
  delete b;
}
#endif

extern "C"
{
  int test_proto_factory()
  {
    UNITY_BEGIN();

#ifdef CONFIG_DAWN_PROTO_DUMMY
    DAWN_RUN_TEST(test_proto_factory_creates_dummy);
    DAWN_RUN_TEST(test_proto_factory_creates_distinct_instances);
#endif

    return UNITY_END();
  }
}
