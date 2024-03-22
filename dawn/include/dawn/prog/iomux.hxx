// dawn/include/dawn/prog/iomux.hxx
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
 * @brief IO multiplexer: routes selected input to one output.
 */
class CProgIOMux : public CProgCommon
{
public:
  enum
  {
    PROG_IOMUX_CFG_FIRST = 0,
    PROG_IOMUX_CFG_CONTROL = 1,
    PROG_IOMUX_CFG_INPUTS = 2,
    PROG_IOMUX_CFG_OUTPUT = 3,
    PROG_IOMUX_CFG_LAST = 31
  };

  explicit CProgIOMux(CDescObject &desc);
  ~CProgIOMux() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "iomux";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_IOMUX, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_IOMUX, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdControl()
  {
    return CProgIOMux::cfgId(false, 1, PROG_IOMUX_CFG_CONTROL);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdInputs(uint16_t size)
  {
    return CProgIOMux::cfgId(false, size, PROG_IOMUX_CFG_INPUTS);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdOutput()
  {
    return CProgIOMux::cfgId(false, 1, PROG_IOMUX_CFG_OUTPUT);
  }

private:
  struct SInput
  {
    CProgIOMux *owner;
    CIOCommon *io;
    SObjectId::ObjectId ioId;
    size_t index;
  };

  CIOCommon *control;
  SObjectId::ObjectId controlId;
  std::vector<SInput> inputs;
  CIOCommon *output;
  SObjectId::ObjectId outputId;
  io_ddata_t *dataBuf;
  size_t currentIndex;
  bool active;
  bool registered;

  static int controlNotifierCb(void *priv, io_ddata_t *data);
  static int inputNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int allocInput(SObjectId::ObjectId ioId);
  int validateShape(CIOCommon *io) const;
  void routeIndex(uint32_t index);
};
} // namespace dawn
