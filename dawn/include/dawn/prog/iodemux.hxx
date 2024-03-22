// dawn/include/dawn/prog/iodemux.hxx
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
 * @brief IO demultiplexer: routes one input to the selected output.
 */
class CProgIODemux : public CProgCommon
{
public:
  enum
  {
    PROG_IODEMUX_CFG_FIRST = 0,
    PROG_IODEMUX_CFG_CONTROL = 1,
    PROG_IODEMUX_CFG_INPUT = 2,
    PROG_IODEMUX_CFG_OUTPUTS = 3,
    PROG_IODEMUX_CFG_LAST = 31
  };

  explicit CProgIODemux(CDescObject &desc);
  ~CProgIODemux() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "iodemux";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_IODEMUX, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_IODEMUX, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdControl()
  {
    return CProgIODemux::cfgId(false, 1, PROG_IODEMUX_CFG_CONTROL);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdInput()
  {
    return CProgIODemux::cfgId(false, 1, PROG_IODEMUX_CFG_INPUT);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdOutputs(uint16_t size)
  {
    return CProgIODemux::cfgId(false, size, PROG_IODEMUX_CFG_OUTPUTS);
  }

private:
  CIOCommon *control;
  SObjectId::ObjectId controlId;
  CIOCommon *input;
  SObjectId::ObjectId inputId;
  std::vector<CIOCommon *> outputs;
  std::vector<SObjectId::ObjectId> outputIds;
  io_ddata_t *dataBuf;
  size_t currentIndex;
  bool active;
  bool registered;

  static int controlNotifierCb(void *priv, io_ddata_t *data);
  static int inputNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int allocOutput(SObjectId::ObjectId ioId);
  int validateShape(CIOCommon *io) const;
  void routeIndex(uint32_t index);
};
} // namespace dawn
