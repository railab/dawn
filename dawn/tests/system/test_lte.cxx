// dawn/tests/system/test_lte.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/system/lte.hxx"
#include "test_common.hxx"

using namespace dawn;

// LTE object id (OBJTYPE_ANY, cls = SYSTEM_CLASS_LTE).

static constexpr auto LTE_ID = CSystemLte::objectId(0);

// Build an LTE descriptor: AUTHTYPE, IPTYPE, REG_TIMEOUT scalar config items.

static void make_lte_bin(uint32_t bin[8], uint32_t auth, uint32_t ip, uint32_t timeout)
{
  bin[0] = LTE_ID;
  bin[1] = 3;
  bin[2] = CSystemLte::cfgIdAuthType();
  bin[3] = auth;
  bin[4] = CSystemLte::cfgIdIpType();
  bin[5] = ip;
  bin[6] = CSystemLte::cfgIdRegTimeout();
  bin[7] = timeout;
}

// Encoding: LTE object is OBJTYPE_ANY with a non-zero class.

static void test_system_lte_objectid()
{
  uint32_t bin[8];

  make_lte_bin(bin, DAWN_LTE_AUTH_PAP, DAWN_LTE_IPTYPE_V4V6, 90);
  CDescObject desc(bin);
  CSystemLte lte(desc);

  TEST_ASSERT_EQUAL(SObjectId::OBJTYPE_ANY, lte.getType());
  TEST_ASSERT_EQUAL(CSystemCommon::SYSTEM_CLASS_LTE, lte.getCls());
  TEST_ASSERT_TRUE(SObjectId::objectIsSystem(reinterpret_cast<SObjectId::UObjectId &>(bin[0])));
}

// configure() accepts valid auth/ip values.

static void test_system_lte_configure_valid()
{
  uint32_t bin[8];

  make_lte_bin(bin, DAWN_LTE_AUTH_CHAP, DAWN_LTE_IPTYPE_V6, 120);
  CDescObject desc(bin);
  CSystemLte lte(desc);

  TEST_ASSERT_EQUAL(OK, lte.configure());
}

// configure() rejects out-of-range auth/ip values.

static void test_system_lte_configure_invalid()
{
  uint32_t bin[8];

  make_lte_bin(bin, 5, DAWN_LTE_IPTYPE_V4, 120); // auth 5 > CHAP
  CDescObject desc1(bin);
  CSystemLte lte1(desc1);
  TEST_ASSERT_EQUAL(-EINVAL, lte1.configure());

  make_lte_bin(bin, DAWN_LTE_AUTH_NONE, 9, 120); // ip 9 > V4V6
  CDescObject desc2(bin);
  CSystemLte lte2(desc2);
  TEST_ASSERT_EQUAL(-EINVAL, lte2.configure());
}

// Config items are reachable through getObjConfig/setObjConfig (the path a
// Config IO uses), and read-write items can be updated at runtime.

static void test_system_lte_config_access()
{
  uint32_t bin[8];
  uint32_t val = 0;

  make_lte_bin(bin, DAWN_LTE_AUTH_PAP, DAWN_LTE_IPTYPE_V4, 90);
  CDescObject desc(bin);
  CSystemLte lte(desc);

  // Read back the descriptor values.

  TEST_ASSERT_EQUAL(OK, lte.getObjConfig(CSystemLte::cfgIdAuthType(), &val, 1));
  TEST_ASSERT_EQUAL(DAWN_LTE_AUTH_PAP, val);

  TEST_ASSERT_EQUAL(OK, lte.getObjConfig(CSystemLte::cfgIdRegTimeout(), &val, 1));
  TEST_ASSERT_EQUAL(90, val);

  // Update auth type and read it back.

  val = DAWN_LTE_AUTH_CHAP;
  TEST_ASSERT_EQUAL(OK, lte.setObjConfig(CSystemLte::cfgIdAuthType(), &val, 1));

  val = 0;
  TEST_ASSERT_EQUAL(OK, lte.getObjConfig(CSystemLte::cfgIdAuthType(), &val, 1));
  TEST_ASSERT_EQUAL(DAWN_LTE_AUTH_CHAP, val);
}

extern "C"
{
  int test_system_lte()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_system_lte_objectid);
    DAWN_RUN_TEST(test_system_lte_configure_valid);
    DAWN_RUN_TEST(test_system_lte_configure_invalid);
    DAWN_RUN_TEST(test_system_lte_config_access);

    return UNITY_END();
  }
}
