// dawn/include/dawn/prog/latest.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/debug.hxx"
#include "dawn/io/ddata.hxx"
#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/process.hxx"

namespace dawn
{
/**
 * @brief Latest-sample cache Program.
 *
 * Subscribes to a notify-capable source IO and stores the latest sample in a
 * output IO so fetch-only protocols can read the most recent value.
 */

class CProgLatest : public CProgProcess
{
public:
  explicit CProgLatest(CDescObject &desc)
    : CProgProcess(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "latest";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_LATEST, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_LATEST, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgLatest::cfgId(false, size, PROG_STATS_CFG_IOBIND);
  }

protected:
  void handle(CIOCommon *output,
              io_ddata_t *data,
              io_ddata_t *ioData,
              io_ddata_t *outputData,
              bool &initsample) override
  {
    int ret;

    (void)ioData;
    (void)outputData;
    (void)initsample;

    ret = output->setData(*data);
    if (ret != OK)
      {
        DAWNERR("failed to cache latest sample %d\n", ret);
      }
  }
};
} // Namespace dawn
