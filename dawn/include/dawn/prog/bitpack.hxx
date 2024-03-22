// dawn/include/dawn/prog/bitpack.hxx
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
 * @brief Bit packer: combines multiple typed inputs into a single
 * packed output bitstream.
 *
 * Each input contributes its logical bitstream starting at the
 * configured bit offset. For BOOL inputs that is one bit per element;
 * for other fixed-width scalar types it is the raw element bit pattern.
 * When any notifying input changes, the packed output is rebuilt and written
 * to the output IO.
 *
 * This is the inverse of CProgBitSplit.
 */

class CProgBitPack : public CProgCommon
{
public:
  enum
  {
    PROG_BITPACK_CFG_FIRST = 0,
    PROG_BITPACK_CFG_INPUTS = 1,
    PROG_BITPACK_CFG_OUTPUT = 2,
    PROG_BITPACK_CFG_LAST = 31
  };

  explicit CProgBitPack(CDescObject &desc);

  ~CProgBitPack() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "bitpack";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_BITPACK, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_BITPACK, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdInputs(uint16_t size)
  {
    return CProgBitPack::cfgId(false, size, PROG_BITPACK_CFG_INPUTS);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdOutput()
  {
    return CProgBitPack::cfgId(false, 1, PROG_BITPACK_CFG_OUTPUT);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t = 0)
  {
    return CProgBitPack::cfgId(false, 0, 0);
  }

private:
  struct SBitInput
  {
    CProgBitPack *owner;
    CIOCommon *io;
    SObjectId::ObjectId ioId;
    uint32_t bit;
    io_ddata_t *currentData;
  };

  CIOCommon *output;             ///< Output IO.
  SObjectId::ObjectId outputId;  ///< Output IO ObjectId.
  std::vector<SBitInput> inputs; ///< Configured bit inputs.
  io_ddata_t *outputData;        ///< Scratch buffer for packed output.
  bool active;                   ///< Activation flag.
  bool registered;               ///< Whether input notifiers are registered.

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int allocInput(SObjectId::ObjectId ioId, uint32_t bit);
  void updateOutput();
};
} // Namespace dawn
