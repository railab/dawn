// dawn/include/dawn/prog/manytoone.hxx
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
 * @brief Many-to-one bridge: forwards the last changed input to one output.
 */
class CProgManyToOne : public CProgCommon
{
public:
  enum
  {
    PROG_MANYTOONE_CFG_FIRST = 0,
    PROG_MANYTOONE_CFG_INPUTS = 1,
    PROG_MANYTOONE_CFG_OUTPUT = 2,
    PROG_MANYTOONE_CFG_LAST = 31
  };

  explicit CProgManyToOne(CDescObject &desc);
  ~CProgManyToOne() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "manytoone";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_MANYTOONE, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_MANYTOONE,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdInputs(uint16_t size)
  {
    return CProgManyToOne::cfgId(false, size, PROG_MANYTOONE_CFG_INPUTS);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdOutput()
  {
    return CProgManyToOne::cfgId(false, 1, PROG_MANYTOONE_CFG_OUTPUT);
  }

private:
  struct SInput
  {
    CProgManyToOne *owner;
    CIOCommon *io;
    SObjectId::ObjectId ioId;
  };

  std::vector<SInput> inputs;
  CIOCommon *output;
  SObjectId::ObjectId outputId;
  io_ddata_t *dataBuf;
  bool active;
  bool registered;

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int allocInput(SObjectId::ObjectId ioId);
  int validateShape(CIOCommon *io) const;
  void writeInput(size_t index);
};
} // namespace dawn
