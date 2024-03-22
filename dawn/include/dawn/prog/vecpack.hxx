// dawn/include/dawn/prog/vecpack.hxx
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
class CIOVirt;

/**
 * @brief Vector packer: combines multiple IO values into one vector IO.
 *
 * Inputs are copied in descriptor order into a cached output vector. When any
 * notifying input changes, the full cached vector is written to the output.
 */

class CProgVecPack : public CProgCommon
{
public:
  enum
  {
    PROG_VECPACK_CFG_FIRST = 0,
    PROG_VECPACK_CFG_INPUTS = 1,
    PROG_VECPACK_CFG_OUTPUT = 2,
    PROG_VECPACK_CFG_LAST = 31
  };

  explicit CProgVecPack(CDescObject &desc);
  ~CProgVecPack() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "vecpack";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_VECPACK, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_VECPACK, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdInputs(uint16_t size)
  {
    return CProgVecPack::cfgId(false, size, PROG_VECPACK_CFG_INPUTS);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdOutput()
  {
    return CProgVecPack::cfgId(false, 1, PROG_VECPACK_CFG_OUTPUT);
  }

private:
  struct SVecInput
  {
    CProgVecPack *owner;
    CIOCommon *io;
    SObjectId::ObjectId ioId;
    io_ddata_t *data;
    size_t offset;
    bool usesSetCallback;
  };

  std::vector<SVecInput> inputs;
  CIOCommon *output;
  SObjectId::ObjectId outputId;
  io_ddata_t *outputData;
  io_ddata_t *lastOutputData;
  bool active;
  bool registered;

  static int ioNotifierCb(void *priv, io_ddata_t *data);
  static void ioSetCb(CIOVirt *io, void *priv);

  int configureDesc(const CDescObject &desc);
  int allocInput(SObjectId::ObjectId ioId);
  void updateOutput();
};
} // namespace dawn
