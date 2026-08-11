// dawn/include/dawn/prog/saturate.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>
#include <inttypes.h>

#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;
struct io_ddata_t;

/**
 * @brief Saturating limiter: clamps input samples into a configured range.
 *
 * Either bound may be omitted, in which case the data type limit is used.
 * The output type may be narrower than the input: clamping into a range the
 * output can hold is what makes the narrowing store safe.
 */

class CProgSaturate : public CProgCommon
{
public:
  enum
  {
    PROG_SATURATE_CFG_FIRST = 0,  ///< reserved
    PROG_SATURATE_CFG_INPUT = 1,  ///< Input I/O
    PROG_SATURATE_CFG_OUTPUT = 2, ///< Output I/O
    PROG_SATURATE_CFG_MIN = 3,    ///< Lower bound
    PROG_SATURATE_CFG_MAX = 4,    ///< Upper bound
    PROG_SATURATE_CFG_LAST = 31   ///< reserved
  };

  explicit CProgSaturate(CDescObject &desc);

  ~CProgSaturate() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "saturate";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;
  bool hasThread() const override;
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

  constexpr static SObjectCfg::ObjectCfgId cfgIdInput()
  {
    return CProgSaturate::cfgId(false, 1, PROG_SATURATE_CFG_INPUT);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdOutput()
  {
    return CProgSaturate::cfgId(false, 1, PROG_SATURATE_CFG_OUTPUT);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdMin(bool rw = false)
  {
    return CProgSaturate::cfgId(rw, 1, PROG_SATURATE_CFG_MIN);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdMax(bool rw = false)
  {
    return CProgSaturate::cfgId(rw, 1, PROG_SATURATE_CFG_MAX);
  }

private:
  CIOCommon *input;          ///< Input IO.
  CIOCommon *output;         ///< Output IO.
  SObjectId::ObjectId inId;  ///< Input IO ObjectId.
  SObjectId::ObjectId outId; ///< Output IO ObjectId.
  io_ddata_t *inputData;     ///< Scratch buffer for non-notify reads.
  io_ddata_t *outputData;    ///< Scratch buffer for the clamped output.
  uint32_t minRaw;           ///< Lower bound as stored in the descriptor.
  uint32_t maxRaw;           ///< Upper bound as stored in the descriptor.
  int64_t lo;                ///< Decoded lower bound.
  int64_t hi;                ///< Decoded upper bound.
  size_t batch;              ///< Output batch count (1 when not batched).
  int dtype;                 ///< Input data type.
  int outDtype;              ///< Output data type (may be narrower).
  bool hasMin;               ///< Whether a lower bound is configured.
  bool hasMax;               ///< Whether an upper bound is configured.
  bool active;               ///< Activation flag.
  bool registered;           ///< Whether the input notifier is registered.

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int resolveBounds();
  void handle(io_ddata_t *data);
};
} // Namespace dawn
