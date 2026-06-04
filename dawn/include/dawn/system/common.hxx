// dawn/include/dawn/system/common.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/object.hxx"

namespace dawn
{
/**
 * @brief Base class for OBJTYPE_ANY configuration objects.
 *
 * SYSTEM objects hold a subsystem's settings as configuration items and own
 * their lifecycle (start/stop). They are not data points: there is no
 * getData/setData. Parameters are reached through a Config IO (which proxies
 * getObjConfig/setObjConfig); start()/stop() can be driven by a Control IO.
 *
 * Class 0 is the descriptor metadata object, so SYSTEM object classes start at
 * SYSTEM_CLASS_FIRST. Multiple instances of one class differ by the priv field.
 */

class CSystemCommon : public CObject
{
public:
  enum
  {
    SYSTEM_CLASS_FIRST = 1,
    SYSTEM_CLASS_LTE = 1,   ///< LTE / cellular connectivity settings
    SYSTEM_CLASS_LAST = 511 ///< 9-bit class field
  };

  explicit CSystemCommon(CDescObject &desc)
    : CObject(desc)
  {
  }

  ~CSystemCommon() override = default;

  /**
   * @brief Build a config-item id for a dev object.
   *
   * @param cls   SYSTEM class (SYSTEM_CLASS_*).
   * @param dtype Data type (SObjectId::DTYPE_*).
   * @param rw    Read-write flag.
   * @param size  Size in 32-bit words.
   * @param id    Config item id.
   */

  constexpr static SObjectCfg::ObjectCfgId cfgId(uint16_t cls,
                                                 uint8_t dtype,
                                                 bool rw,
                                                 uint16_t size,
                                                 uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_ANY, cls, dtype, rw, size, id);
  }
};
} // Namespace dawn
