// dawn/include/dawn/prog/bitsplit.hxx
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
class CIOCommon;

/**
 * @brief Bit demultiplexer: extracts configurable bit slices from the
 * input into separate outputs.
 *
 * Each output receives a logical bit slice starting at the configured bit
 * position. For BOOL outputs that is one bit per element; for other
 * fixed-width scalar types it is the raw element bit pattern.
 */

class CProgBitSplit : public CProgCommon
{
public:
  enum
  {
    PROG_BITSPLIT_CFG_FIRST = 0,
    PROG_BITSPLIT_CFG_IOBIND = 1,
    PROG_BITSPLIT_CFG_BITS = 2,
    PROG_BITSPLIT_CFG_LAST = 31
  };

  explicit CProgBitSplit(CDescObject &desc);
  ~CProgBitSplit() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "bitsplit";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_BITSPLIT, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_BITSPLIT,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgBitSplit::cfgId(false, size, PROG_BITSPLIT_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdBits(uint16_t size = 2)
  {
    return CProgBitSplit::cfgId(false, size, PROG_BITSPLIT_CFG_BITS);
  }

private:
  struct SBitSplitBind
  {
    CProgBitSplit *owner;
    SObjectId::ObjectId sourceId;
    SObjectId::ObjectId outputId;
    CIOCommon *source;
    CIOCommon *output;
    io_ddata_t *sourceData;
    io_ddata_t *outputData;
    uint32_t bit;
  };

  std::vector<SBitSplitBind *> binds; ///< Configured source/output binds.
  std::vector<uint32_t> bitPositions; ///< Configured start bit positions.
  bool active;                        ///< Activation flag.
  bool registered;                    ///< Whether source notifiers are registered.

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int allocBind(SObjectId::ObjectId sourceId, SObjectId::ObjectId outputId);
  void refreshSource(SObjectId::ObjectId sourceId);
  void updateBind(SBitSplitBind *bind, io_ddata_t *data);
};
} // Namespace dawn
