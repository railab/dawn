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

struct SLteParams
{
  const char *apn;      ///< APN name (empty/null: use network default)
  const char *username; ///< APN username (empty/null: none)
  const char *password; ///< APN password (empty/null: none)
  uint8_t auth_type;    ///< DAWN_LTE_AUTH_*
  uint8_t ip_type;      ///< DAWN_LTE_IPTYPE_*
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
