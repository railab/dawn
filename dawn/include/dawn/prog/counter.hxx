// dawn/include/dawn/prog/counter.hxx
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
 * @brief Event counter with configurable wrap-around.
 *
 * On each rising edge of the input, increments the internal counter
 * by the configured step.  The counter wraps back to the minimum value
 * when it would exceed the maximum.  The current count is written to the
 * output IO on every update.
 */

class CProgCounter : public CProgCommon
{
public:
  enum
  {
    PROG_COUNTER_CFG_FIRST = 0,
    PROG_COUNTER_CFG_IOBIND = 1,
    PROG_COUNTER_CFG_PARAMS = 2,
    PROG_COUNTER_CFG_LAST = 31
  };

  explicit CProgCounter(CDescObject &desc);
  ~CProgCounter() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "counter";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_COUNTER, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_COUNTER, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgCounter::cfgId(false, size, PROG_COUNTER_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdParams(uint16_t size = 4)
  {
    return CProgCounter::cfgId(false, size, PROG_COUNTER_CFG_PARAMS);
  }

private:
  struct SCounterBind
  {
    CProgCounter *owner;
    SObjectId::ObjectId sourceId;
    SObjectId::ObjectId outputId;
    CIOCommon *source;
    CIOCommon *output;
    io_ddata_t *sourceData;
    io_sdata_t<uint32_t, 1, 1> outputData;
    uint32_t prevInput;
  };

  std::vector<SCounterBind *> binds; ///< Configured source/output binds.
  uint32_t countMin;                 ///< Minimum count value (inclusive).
  uint32_t countMax;                 ///< Maximum count value (inclusive).
  uint32_t countStep;                ///< Increment per event.
  uint32_t countInit;                ///< Initial/reset count value.
  uint32_t count;                    ///< Current count value.
  bool active;                       ///< Activation flag.
  bool registered;                   ///< Whether source notifiers are registered.

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int allocBind(SObjectId::ObjectId sourceId, SObjectId::ObjectId outputId);
  uint32_t readInput(SCounterBind *bind, io_ddata_t *data);
  void writeCount(SCounterBind *bind);
  void refresh(SCounterBind *bind, io_ddata_t *data);
};
} // Namespace dawn
