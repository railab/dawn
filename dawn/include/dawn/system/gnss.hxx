// dawn/include/dawn/system/gnss.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/thread.hxx"
#include "dawn/system/common.hxx"

namespace dawn
{
/**
 * @brief GNSS coexistence policy modes (GNSS_CFG_MODE values).
 */

enum
{
  DAWN_GNSS_MODE_OFF = 0,    ///< Manager idle; do not touch the radio knobs.
  DAWN_GNSS_MODE_HYBRID = 1, ///< LTE first, then borrow the radio in bounded
                             ///< windows for a cold fix, always handing it
                             ///< back so LTE stays reachable (indoors too).
};

/**
 * @brief GNSS coexistence manager.
 *
 * The companion to the LTE manager (CSystemLte). On a single radio the modem
 * time-shares LTE and GNSS; the kernel only provides the *mechanisms* (GNSS
 * priority via SNIOC_GNSS_SET_PRIORITY, LTE power-save via the LTE API). This
 * userspace manager owns the *policy*: a monitor thread watches GNSS for a fix
 * and drives those knobs according to the configured mode. It is the place to
 * implement and tune coexistence without touching the kernel.
 */

class CSystemGnss
  : public CSystemCommon
  , public CThreadedObject
{
public:
  /** @brief Config item ids (cls = SYSTEM_CLASS_GNSS). */

  enum
  {
    GNSS_CFG_FIRST = 0,
    GNSS_CFG_MODE = 1,        ///< DAWN_GNSS_MODE_* (DTYPE_UINT8)
    GNSS_CFG_SETTLE = 2,      ///< LTE-first grace, seconds (DTYPE_UINT32)
    GNSS_CFG_ACQUIRE = 3,     ///< Bounded fix window, seconds (DTYPE_UINT32)
    GNSS_CFG_RETRY = 4,       ///< Cooldown between attempts, s (DTYPE_UINT32)
    GNSS_CFG_MAXATTEMPTS = 5, ///< No-fix windows before give-up (DTYPE_UINT32)
    GNSS_CFG_REARM = 6,       ///< Re-arm delay after give-up, s (DTYPE_UINT32)
    GNSS_CFG_ENABLED = 7,     ///< Runtime on/off switch (DTYPE_BOOL)
    GNSS_CFG_LAST = 31
  };

  /** @brief Build this object's ObjectId for a given instance. */

  constexpr static SObjectId::ObjectId objectId(uint16_t inst = 0)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_ANY, SYSTEM_CLASS_GNSS, SObjectId::DTYPE_ANY, 0, inst);
  }

  /** @brief Config-item id helper (cls = SYSTEM_CLASS_GNSS). */

  constexpr static SObjectCfg::ObjectCfgId cfgIdMode(bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_GNSS, SObjectId::DTYPE_UINT8, rw, 1, GNSS_CFG_MODE);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdSettle(bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_GNSS, SObjectId::DTYPE_UINT32, rw, 1, GNSS_CFG_SETTLE);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdAcquire(bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_GNSS, SObjectId::DTYPE_UINT32, rw, 1, GNSS_CFG_ACQUIRE);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdRetry(bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_GNSS, SObjectId::DTYPE_UINT32, rw, 1, GNSS_CFG_RETRY);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdMaxAttempts(bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_GNSS, SObjectId::DTYPE_UINT32, rw, 1, GNSS_CFG_MAXATTEMPTS);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdRearm(bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_GNSS, SObjectId::DTYPE_UINT32, rw, 1, GNSS_CFG_REARM);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdEnabled(bool rw = true)
  {
    return cfgId(SYSTEM_CLASS_GNSS, SObjectId::DTYPE_BOOL, rw, 1, GNSS_CFG_ENABLED);
  }

  explicit CSystemGnss(CDescObject &desc);

  int configure() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "gnss";
  }
#endif

protected:
  int doStart() override;
  int doStop() override;

  /** @brief React to runtime config writes (e.g. enable/disable over LwM2M). */

  int onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len) override;

private:
  uint8_t mode;         ///< DAWN_GNSS_MODE_*
  bool enabled;         ///< Runtime on/off (GNSS_CFG_ENABLED).
  uint32_t settle;      ///< LTE-first grace before first GNSS attempt (s).
  uint32_t acquire;     ///< Bounded fix-acquisition window (seconds).
  uint32_t retry;       ///< LTE-reachable cooldown between attempts (seconds).
  uint32_t maxAttempts; ///< No-fix windows before give-up (0 = never).
  uint32_t rearm;       ///< Re-arm delay after give-up (seconds).
  int fd;               ///< GNSS device file descriptor used for monitoring.

  /** @brief Load parameters from descriptor config items over Kconfig defaults. */

  void loadParams();

  /** @brief Open the GNSS device and spawn the monitor thread (idempotent). */

  int startMonitor();

  /** @brief Stop the monitor thread and close the GNSS device (idempotent). */

  void stopMonitor();

  /** @brief Monitor-thread body implementing the coexistence policy. */

  void monitor();
};
} // Namespace dawn
