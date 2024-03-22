// dawn/include/dawn/prog/vecsplit.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <inttypes.h>
#include <vector>

#include "dawn/io/ddata.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
class CIOCommon;

/**
 * @brief Vector splitter: splits one vector IO into multiple outputs.
 *
 * Outputs receive contiguous slices in descriptor order. Deferred virtual
 * outputs are initialized as scalar outputs by default.
 */

class CProgVecSplit : public CProgCommon
{
public:
  enum
  {
    PROG_VECSPLIT_CFG_FIRST = 0,
    PROG_VECSPLIT_CFG_SOURCE = 1,
    PROG_VECSPLIT_CFG_OUTPUTS = 2,
    PROG_VECSPLIT_CFG_LAST = 31
  };

  explicit CProgVecSplit(CDescObject &desc);
  ~CProgVecSplit() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "vecsplit";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_VECSPLIT, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_VECSPLIT,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdSource()
  {
    return CProgVecSplit::cfgId(false, 1, PROG_VECSPLIT_CFG_SOURCE);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdOutputs(uint16_t size)
  {
    return CProgVecSplit::cfgId(false, size, PROG_VECSPLIT_CFG_OUTPUTS);
  }

private:
  struct SVecOutput
  {
    CIOCommon *io;
    SObjectId::ObjectId ioId;
    io_ddata_t *data;
    size_t offset;
  };

  CIOCommon *source;
  SObjectId::ObjectId sourceId;
  io_ddata_t *sourceData;
  std::vector<SVecOutput> outputs;
  bool active;
  bool registered;

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int allocOutput(SObjectId::ObjectId ioId);
  void updateOutputs(io_ddata_t *data);
};
} // namespace dawn
