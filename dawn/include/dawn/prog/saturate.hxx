// dawn/include/dawn/prog/saturate.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <atomic>
#include <cstdint>

#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"
#include "dawn/prog/process.hxx"

namespace dawn
{
/**
 * @brief Saturating limiter: clamps input samples into a [min, max] range.
 *
 * Every sample of a batched source is clamped in the source type and written
 * to an output of the same type.
 */

class CProgSaturate : public CProgProcess
{
public:
  enum
  {
    PROG_SATURATE_CFG_MIN = 2, ///< Lower bound
    PROG_SATURATE_CFG_MAX = 3, ///< Upper bound
  };

  explicit CProgSaturate(CDescObject &desc)
    : CProgProcess(desc)
    , minRaw(0)
    , maxRaw(0)
    , hasMin(false)
    , hasMax(false)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "saturate";
  }
#endif

  int onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len) override;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_SATURATE, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_SATURATE,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgSaturate::cfgId(false, size, PROG_STATS_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdMin(bool rw = false)
  {
    return CProgSaturate::cfgId(rw, 1, PROG_SATURATE_CFG_MIN);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdMax(bool rw = false)
  {
    return CProgSaturate::cfgId(rw, 1, PROG_SATURATE_CFG_MAX);
  }

protected:
  int configureExtraCfgItem(const CDescObject &desc,
                            const SObjectCfg::SObjectCfgItem *item,
                            size_t &offset) override;

  bool isBatchAware() const override
  {
    return true;
  }

  void handle(CIOCommon *output,
              io_ddata_t *data,
              io_ddata_t *ioData,
              io_ddata_t *outputData,
              bool &initsample) override;

private:
  std::atomic<uint32_t> minRaw; ///< Lower bound word, source type encoding.
  std::atomic<uint32_t> maxRaw; ///< Upper bound word, source type encoding.
  bool hasMin;                  ///< Lower bound configured.
  bool hasMax;                  ///< Upper bound configured.

  /** @brief Clamp the latest output batches of data into outputData. */

  template<typename T>
  void clamp(io_ddata_t *data, io_ddata_t *outputData, T lo, T hi);
};
} // Namespace dawn
