// dawn/include/dawn/proto/common.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdlib>

#include "dawn/common/bindable.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Base class for all protocol implementations.
 *
 * Provides common interface for all protocols (Serial, Modbus, CAN, BLE,
 * Shell, etc.).
 */

class CProtoCommon : public CBindableObject
{
public:
  /** @brief Protocol object class types. */

  enum
  {
    /** @brief Wildcard/any protocol class (for validation/queries). */

    PROTO_CLASS_ANY = 0,

    /** @brief BLE Peripheral using Apache NimBLE stack. */

    PROTO_CLASS_NIMBLE_PRPH = 1,

    /** @brief NxScope real-time visualization (dummy interface). */

    PROTO_CLASS_NXSCOPE_DUMMY = 10,

    /** @brief Dummy protocol (test/helper). */

    PROTO_CLASS_DUMMY = 12,

    /** @brief NxScope real-time visualization (serial interface). */

    PROTO_CLASS_NXSCOPE_SERIAL = 11,

    /** @brief NxScope real-time visualization (UDP interface). */

    PROTO_CLASS_NXSCOPE_UDP = 13,

    /** @brief Interactive shell on stdin/stdout. */

    PROTO_CLASS_SHELL_STD = 15,

    /** @brief Interactive shell on serial port. */

    PROTO_CLASS_SHELL_SERIAL = 16,

    /** @brief Compact binary protocol over serial port. */

    PROTO_CLASS_SERIAL = 17,

    /** @brief Modbus RTU (serial implementation). */

    PROTO_CLASS_MODBUS_RTU = 18,

    /** @brief Modbus TCP (network implementation). */

    PROTO_CLASS_MODBUS_TCP = 19,

    /** @brief CAN bus protocol. */

    PROTO_CLASS_CAN = 20,

    /** @brief UDP-based protocol. */

    PROTO_CLASS_UDP = 22,

    /** @brief FIFO-based local IPC protocol. */

    PROTO_CLASS_IPC = 23,

    /** @brief User-defined protocols start from here. */

    PROTO_CLASS_USER_START = 510,

    /** @brief Placeholder for user protocol. */

    PROTO_CLASS_USER = 511,

    /** @brief Last protocol class (sentinel value). */

    PROTO_CLASS_LAST
  } typedef EProtoClass;

  static_assert(PROTO_CLASS_LAST - 1 <= SObjectId::CLS_MAX);

  /** @brief Common configuration IDs. */

  enum
  {
    PROTO_CFG_FIRST = 0,
    PROTO_CFG_LAST = 31
  };

  /**
   * @brief Constructor.
   *
   * Initializes base PROT object with metadata from descriptor.
   *
   * @param[in] desc Protocol descriptor with configuration.
   */

  explicit CProtoCommon(CDescObject &desc);
};
} // Namespace dawn
