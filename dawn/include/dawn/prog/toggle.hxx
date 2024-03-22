// dawn/include/dawn/prog/toggle.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>
#include <inttypes.h>
#include <vector>

#include "dawn/io/sdata.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
class CIOCommon;
class io_ddata_t;

/**
 * @brief Toggle/latch: flips output between two configured values on each
 * rising edge of the input.
 *
 * Stores persistent state across samples.  The initial output is the
 * off-value; the first rising edge transitions to the on-value, subsequent
 * edges toggle between the two.
 */

class CProgToggle : public CProgCommon
{
public:
  enum
  {
    PROG_TOGGLE_CFG_FIRST = 0,
    PROG_TOGGLE_CFG_IOBIND = 1,
    PROG_TOGGLE_CFG_VALUES = 2,
    PROG_TOGGLE_CFG_LAST = 31
  };

  explicit CProgToggle(CDescObject &desc);
  ~CProgToggle() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "toggle";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;
  bool hasThread() const override;
  int trigger(uint8_t cmd) override;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_TOGGLE, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_TOGGLE, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgToggle::cfgId(false, size, PROG_TOGGLE_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdValues(uint16_t size = 2)
  {
    return CProgToggle::cfgId(false, 2, PROG_TOGGLE_CFG_VALUES);
  }

private:
  struct SToggleBind
  {
    CProgToggle *owner;
    SObjectId::ObjectId sourceId;
    SObjectId::ObjectId outputId;
    CIOCommon *source;
    CIOCommon *output;
    io_ddata_t *sourceData;
    io_sdata_t<uint32_t, 1, 1> outputData;
    uint32_t prevInput;
  };

  std::vector<SToggleBind *> binds; ///< Configured source/output binds.
  uint32_t onVal;                   ///< Output value when toggled on.
  uint32_t offVal;                  ///< Output value when toggled off.
  uint32_t curVal;                  ///< Current output value (offVal or onVal).
  bool active;                      ///< Activation flag.
  bool registered;                  ///< Whether source notifiers are registered.

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int allocBind(SObjectId::ObjectId sourceId, SObjectId::ObjectId outputId);
  uint32_t readInput(SToggleBind *bind, io_ddata_t *data);
  void writeValue(SToggleBind *bind);
  void refresh(SToggleBind *bind, io_ddata_t *data);
};
} // Namespace dawn
