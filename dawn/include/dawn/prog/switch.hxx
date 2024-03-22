// dawn/include/dawn/prog/switch.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>
#include <inttypes.h>
#include <vector>

#include "dawn/io/ddata.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;

/**
 * @brief Multi-input AND-gate switch: writes on/off commands to a target IO
 * when all control inputs match their configured values.
 *
 * Notify-capable control inputs trigger evaluation.  When every input equals
 * its configured match value the switch writes an on-command to the target;
 * otherwise it writes the off-command. State tracking avoids redundant writes.
 */

class CProgSwitch : public CProgCommon
{
public:
  enum
  {
    PROG_SWITCH_CFG_FIRST = 0,
    PROG_SWITCH_CFG_INPUTS = 1,
    PROG_SWITCH_CFG_TARGET = 2,
    PROG_SWITCH_CFG_LAST = 31
  };

  explicit CProgSwitch(CDescObject &desc);

  ~CProgSwitch() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "switch";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;
  bool hasThread() const override;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_SWITCH, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_SWITCH, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdInputs(uint16_t size)
  {
    return CProgSwitch::cfgId(false, size, PROG_SWITCH_CFG_INPUTS);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdTarget()
  {
    return CProgSwitch::cfgId(false, 3, PROG_SWITCH_CFG_TARGET);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t = 0)
  {
    return CProgSwitch::cfgId(false, 0, 0);
  }

private:
  /** Per-input state: IO, ObjectId, match value, last known value. */
  struct SSwitchInput
  {
    CProgSwitch *owner;
    CIOCommon *io;
    SObjectId::ObjectId ioId;
    uint32_t match;
    uint32_t current;
  };

  CIOCommon *target;                ///< Resolved target IO.
  SObjectId::ObjectId targetId;     ///< Target IO ObjectId.
  io_ddata_t *iodata;               ///< Buffer for setData writes.
  uint8_t onCmd;                    ///< Command written when AND is true.
  uint8_t offCmd;                   ///< Command written when AND is false.
  std::vector<SSwitchInput> inputs; ///< Configured control inputs.
  bool lastAndState;                ///< Previous AND evaluation result.
  bool active;                      ///< Activation flag.
  bool registered;                  ///< Whether input notifiers are registered.

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int allocInput(SObjectId::ObjectId ioId, uint32_t match);
  bool allInputsMatch() const;
  void evaluate();
};
} // Namespace dawn
