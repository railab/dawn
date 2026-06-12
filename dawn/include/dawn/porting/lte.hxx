// dawn/include/dawn/porting/lte.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>

namespace dawn
{
/**
 * @brief LTE authentication type (target-independent).
 */

enum
{
  DAWN_LTE_AUTH_NONE = 0, ///< No authentication
  DAWN_LTE_AUTH_PAP = 1,  ///< PAP
  DAWN_LTE_AUTH_CHAP = 2, ///< CHAP
};

/**
 * @brief LTE IP type (target-independent).
 */

enum
{
  DAWN_LTE_IPTYPE_V4 = 0,   ///< IPv4
  DAWN_LTE_IPTYPE_V6 = 1,   ///< IPv6
  DAWN_LTE_IPTYPE_V4V6 = 2, ///< IPv4/IPv6
};

/**
 * @brief LTE power-save mode.
 */

enum
{
  DAWN_LTE_PSAVE_NONE = 0, ///< PSM and eDRX disabled (always reachable)
  DAWN_LTE_PSAVE_PSM = 1,  ///< Power Saving Mode
  DAWN_LTE_PSAVE_EDRX = 2, ///< extended DRX
};

/**
 * @brief LTE connection status.
 */

enum
{
  DAWN_LTE_STATUS_DOWN = 0,      ///< Not connected
  DAWN_LTE_STATUS_CONNECTED = 1, ///< Connected (PDN active, IP assigned)
};

/**
 * @brief Parameters used to bring up an LTE data connection.
 *
 * These are filled by the LTE system object from Kconfig defaults and the
 * device descriptor, and consumed by the porting layer which talks to the
 * common (target-independent) NuttX LTE API.
 */

struct SLteQuality
{
  bool valid;   ///< Values are meaningful only when true (RF on, camped).
  int16_t rsrp; ///< Reference Signal Received Power, dBm (-140..0).
  int16_t rsrq; ///< Reference Signal Received Quality, dB (-60..0).
  int16_t sinr; ///< Signal to Interference + Noise Ratio, dB (-128..40).
  int16_t rssi; ///< Received Signal Strength Indicator, dBm.
};

struct SLteCellinfo
{
  bool valid;    ///< Values are meaningful only when true (RF on, camped).
  uint16_t band; ///< Serving E-UTRA band number.
};

struct SLteParams
{
  const char *apn;      ///< APN name (empty/null: use network default)
  const char *username; ///< APN username (empty/null: none)
  const char *password; ///< APN password (empty/null: none)
  uint8_t auth_type;    ///< DAWN_LTE_AUTH_*
  uint8_t ip_type;      ///< DAWN_LTE_IPTYPE_*
  uint8_t psave_mode;   ///< DAWN_LTE_PSAVE_*
  uint32_t reg_timeout; ///< Registration/activation timeout (seconds)
};
} // Namespace dawn

/**
 * @brief Bring up the LTE data connection.
 *
 * Implemented by the porting layer on top of the common NuttX LTE API so it
 * works with any modem that provides an LTE API backend.  No chip-specific
 * dependency is allowed here.
 *
 * @param params Connection parameters.
 * @return OK on success, negative errno on failure.
 */

int lte_port_connect(const struct dawn::SLteParams *params);

/**
 * @brief Tear down the LTE data connection.
 *
 * @return OK on success, negative errno on failure.
 */

int lte_port_disconnect(void);

/**
 * @brief Query the current LTE connection status.
 *
 * @param status Set to one of DAWN_LTE_STATUS_*.
 * @return OK on success, negative errno on failure.
 */

int lte_port_status(uint32_t *status);

/**
 * @brief Read the current LTE signal quality (RSRP/RSRQ/SINR/RSSI).
 *
 * Synchronous query of the modem via the common NuttX LTE API. Values are
 * only meaningful when @p quality->valid is true (RF on and camped on a cell).
 *
 * @param quality Filled with the current signal metrics.
 * @return OK on success, negative errno on failure.
 */

int lte_port_get_quality(struct dawn::SLteQuality *quality);

/**
 * @brief Read serving-cell info (currently the E-UTRA band).
 *
 * Synchronous query of the modem via the common NuttX LTE API. Values are only
 * meaningful when @p info->valid is true (RF on and camped on a cell).
 *
 * @param info Filled with the current cell info.
 * @return OK on success, negative errno on failure.
 */

int lte_port_get_cellinfo(struct dawn::SLteCellinfo *info);

/**
 * @brief Set the LTE power-save mode at runtime.
 *
 * Lets a userspace policy retune reachability vs. power (and free the radio
 * for GNSS) after the connection is up.
 *
 * @param mode One of DAWN_LTE_PSAVE_* (none / PSM / eDRX).
 * @return OK on success, negative errno on failure.
 */

int lte_port_set_psave(uint8_t mode);
