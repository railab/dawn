// dawn/include/dawn/prog/bitmerge.hxx
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
 * @brief Bit merger: packs masked, shifted input values into one word.
 *
 *   out = OR over inputs of ((in & mask) << shift)
 *
 * When one input delivers batched data it is the stream: the merge runs per
 * sample and every batch is published. The rest are read once per batch.
 */

class CProgBitMerge : public CProgCommon
{
public:
  enum
  {
    PROG_BITMERGE_CFG_FIRST = 0,
    PROG_BITMERGE_CFG_INPUTS = 1,
    PROG_BITMERGE_CFG_OUTPUT = 2,
    PROG_BITMERGE_CFG_LAST = 31
  };

  explicit CProgBitMerge(CDescObject &desc);

  ~CProgBitMerge() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "bitmerge";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_BITMERGE, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_BITMERGE,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdInputs(uint16_t size)
  {
    return CProgBitMerge::cfgId(false, size, PROG_BITMERGE_CFG_INPUTS);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdOutput()
  {
    return CProgBitMerge::cfgId(false, 1, PROG_BITMERGE_CFG_OUTPUT);
  }

private:
  struct SMergeInput
  {
    CProgBitMerge *owner;
    CIOCommon *io;
    SObjectId::ObjectId ioId;
    uint32_t shift;
    uint32_t mask;
    io_ddata_t *currentData;
  };

  static constexpr size_t NO_STREAM = static_cast<size_t>(-1);

  CIOCommon *output;               ///< Output IO.
  SObjectId::ObjectId outputId;    ///< Output IO ObjectId.
  std::vector<SMergeInput> inputs; ///< Configured merge inputs.
  io_ddata_t *outputData;          ///< Scratch buffer for the merged output.
  io_ddata_t *lastOutputData;      ///< Last published output (scalar dedup).
  size_t streamIdx;                ///< Index of the batched input, or NO_STREAM.
  size_t batch;                    ///< Output batch count (1 when scalar).
  bool active;                     ///< Activation flag.
  bool registered;                 ///< Whether input notifiers are registered.

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  void mergeBatch(const void *in, void *out, size_t items, const SMergeInput *src, uint32_t orval);

  int configureDesc(const CDescObject &desc);
  int allocInput(SObjectId::ObjectId ioId, uint32_t shift, uint32_t mask);
  int checkFieldLayout() const;
  uint32_t mergeConstant(size_t skip);
  void updateScalar();
  void updateStream(io_ddata_t *data);
};
} // Namespace dawn
