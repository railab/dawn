// dawn/include/dawn/system/lte.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/system/common.hxx"
#include "dawn/porting/lte.hxx"

namespace dawn
{
/**
 * @brief LTE connectivity object (OBJTYPE_ANY, cls = SYSTEM_CLASS_LTE).
 *
 * Holds the data-connection parameters as config items (reachable from a
 * Config IO) and owns the connection lifecycle: start() connects, stop()
 * disconnects, through the common NuttX LTE API. No data path.
 */

class CSystemLte : public CSystemCommon
{
public:
  /** @brief Config item ids (cls = SYSTEM_CLASS_LTE). */

  enum
  {
    LTE_CFG_FIRST = 0,
    LTE_CFG_APN = 1,         ///< APN (DTYPE_CHAR)
    LTE_CFG_USERNAME = 2,    ///< APN username (DTYPE_CHAR)
    LTE_CFG_PASSWORD = 3,    ///< APN password (DTYPE_CHAR)
    LTE_CFG_AUTHTYPE = 4,    ///< DAWN_LTE_AUTH_* (DTYPE_UINT8)
    LTE_CFG_IPTYPE = 5,      ///< DAWN_LTE_IPTYPE_* (DTYPE_UINT8)
    LTE_CFG_REG_TIMEOUT = 6, ///< Registration timeout, seconds (DTYPE_UINT32)
    LTE_CFG_LAST = 31
  };

  /** @brief Parameter string buffer sizes. */

  enum
  {
    LTE_APN_SIZE = 64,
    LTE_USER_SIZE = 32,
    LTE_PASS_SIZE = 32
  };

  /** @brief Build this object's ObjectId for a given instance. */

  constexpr static SObjectId::ObjectId objectId(uint16_t inst = 0)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_ANY, SYSTEM_CLASS_LTE, SObjectId::DTYPE_ANY, 0, inst);
  }

  /** @brief Config-item id helpers (cls = SYSTEM_CLASS_LTE). */

  constexpr static SObjectCfg::ObjectCfgId cfgIdApn(uint16_t words, bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_LTE, SObjectId::DTYPE_CHAR, rw, words, LTE_CFG_APN);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdUsername(uint16_t words, bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_LTE, SObjectId::DTYPE_CHAR, rw, words, LTE_CFG_USERNAME);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPassword(uint16_t words, bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_LTE, SObjectId::DTYPE_CHAR, rw, words, LTE_CFG_PASSWORD);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdAuthType(bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_LTE, SObjectId::DTYPE_UINT8, rw, 1, LTE_CFG_AUTHTYPE);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIpType(bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_LTE, SObjectId::DTYPE_UINT8, rw, 1, LTE_CFG_IPTYPE);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdRegTimeout(bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_LTE, SObjectId::DTYPE_UINT32, rw, 1, LTE_CFG_REG_TIMEOUT);
  }

  explicit CSystemLte(CDescObject &desc);

  int configure() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "lte";
  }
#endif

protected:
  int doStart() override;
  int doStop() override;

private:
  char apn[LTE_APN_SIZE];
  char username[LTE_USER_SIZE];
  char password[LTE_PASS_SIZE];
  uint8_t auth_type;
  uint8_t ip_type;
  uint8_t psave_mode;
  uint32_t reg_timeout;

  /** @brief Load parameters from descriptor config items over Kconfig defaults. */

  void loadParams();
};
} // Namespace dawn
