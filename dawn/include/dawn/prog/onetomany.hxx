// dawn/include/dawn/prog/onetomany.hxx
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
 * @brief One-to-many bridge: forwards one input to many outputs.
 */
class CProgOneToMany : public CProgCommon
{
public:
  enum
  {
    PROG_ONETOMANY_CFG_FIRST = 0,
    PROG_ONETOMANY_CFG_INPUT = 1,
    PROG_ONETOMANY_CFG_OUTPUTS = 2,
    PROG_ONETOMANY_CFG_LAST = 31
  };

  explicit CProgOneToMany(CDescObject &desc);
  ~CProgOneToMany() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "onetomany";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_ONETOMANY, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_ONETOMANY,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdInput()
  {
    return CProgOneToMany::cfgId(false, 1, PROG_ONETOMANY_CFG_INPUT);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdOutputs(uint16_t size)
  {
    return CProgOneToMany::cfgId(false, size, PROG_ONETOMANY_CFG_OUTPUTS);
  }

private:
  CIOCommon *input;
  SObjectId::ObjectId inputId;
  std::vector<CIOCommon *> outputs;
  std::vector<SObjectId::ObjectId> outputIds;
  io_ddata_t *dataBuf;
  bool active;
  bool registered;

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int allocOutput(SObjectId::ObjectId ioId);
  int validateShape(CIOCommon *io) const;
  void writeOutputs(io_ddata_t *data);
};
} // namespace dawn
