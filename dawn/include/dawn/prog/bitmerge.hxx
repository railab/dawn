// dawn/include/dawn/prog/bitmerge.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>
#include <inttypes.h>
#include <mutex>
#include <vector>

#include "dawn/io/ddata.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;

/**
 * @brief Bit merger: out = OR over inputs of ((in & mask) << shift).
 *
 * An optional batched input is merged per sample; the rest once per batch.
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
  size_t streamIdx;                ///< Index of the batched input, or NO_STREAM.
  size_t batch;                    ///< Output batch count (1 unless a virt output).
  bool active;                     ///< Activation flag.
  bool registered;                 ///< Whether input notifiers are registered.
  std::mutex mergeLock;            ///< Serializes merge+publish across notifier threads.

  /** @brief Input notification: merge a batch or rebuild the scalar. */

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  /** @brief Merge items samples of the batched input into out, OR orval. */

  void mergeBatch(const void *in, void *out, size_t items, const SMergeInput *src, uint32_t orval);

  /** @brief Parse inputs and output from the descriptor. */

  int configureDesc(const CDescObject &desc);

  /** @brief Add one input with its field shift and mask. */

  int allocInput(SObjectId::ObjectId ioId, uint32_t shift, uint32_t mask);

  /** @brief Reject empty, overlapping or out-of-word fields for width bits. */

  int checkFieldLayout(uint32_t width) const;

  /**
   * @brief Read and merge all inputs except skip into acc.
   *
   * @return OK, or the getData() error of a slow input.
   */

  int mergeConstant(size_t skip, uint32_t &acc);

  /** @brief Rebuild and publish the scalar output (no batched input). */

  void updateScalar();

  /** @brief Merge and publish one batch of the batched input. */

  void updateStream(io_ddata_t *data);
};
} // Namespace dawn
