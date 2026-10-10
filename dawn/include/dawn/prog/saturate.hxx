// dawn/include/dawn/prog/saturate.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>
#include <inttypes.h>
#include <vector>

#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"
#include "dawn/prog/process.hxx"

namespace dawn
{
/**
 * @brief Saturating limiter: clamps input samples into a [min, max] range.
 *
 * Bounds are decoded once into the input type and every sample is clamped in
 * that type, batches included.
 */

class CProgSaturate : public CProgProcess
{
public:
  enum
  {
    PROG_SATURATE_CFG_MIN = 2, ///< Lower bound
    PROG_SATURATE_CFG_MAX = 3, ///< Upper bound
  };

  explicit CProgSaturate(CDescObject &desc);

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "saturate";
  }
#endif

  int deinit() override;
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

  int bindStateAlloc(CIOCommon *src,
                     CIOCommon *output,
                     io_ddata_t *ioData,
                     io_ddata_t *outputData,
                     SBindState **state) override;

  void handle(CIOCommon *output,
              io_ddata_t *data,
              io_ddata_t *ioData,
              io_ddata_t *outputData,
              bool &initsample) override;

  void handleWithState(CIOCommon *output,
                       io_ddata_t *data,
                       io_ddata_t *ioData,
                       io_ddata_t *outputData,
                       bool &initsample,
                       void *state) override;

private:
  /** @brief Per-binding types and bounds encoded like the input type. */

  struct SState final : public SBindState
  {
    uint8_t dtype = 0;          ///< Input data type.
    uint8_t outDtype = 0;       ///< Output data type (may be narrower).
    uint32_t lo = 0;            ///< Lower bound word.
    uint32_t hi = 0;            ///< Upper bound word.
  };

  uint32_t minRaw;              ///< Lower bound as stored in the descriptor.
  uint32_t maxRaw;              ///< Upper bound as stored in the descriptor.
  bool hasMin;                  ///< Whether a lower bound is configured.
  bool hasMax;                  ///< Whether an upper bound is configured.
  std::vector<SState *> states; ///< Binding states, owned by CProgProcess.

  /**
   * @brief Resolve bounds for one binding.
   *
   * @param[in,out] st Binding state; lo/hi are written only on success.
   * @param[in] minw Lower bound word, used when hasmin is true.
   * @param[in] maxw Upper bound word, used when hasmax is true.
   * @return OK, -ENOTSUP for unsupported types, -ERANGE or -EINVAL for bad
   *         bounds.
   */

  static int resolveBounds(SState &st, uint32_t minw, bool hasmin, uint32_t maxw, bool hasmax);
};
} // Namespace dawn
