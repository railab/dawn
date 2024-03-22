// dawn/include/dawn/prog/statsavg.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/ddata.hxx"
#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"
#include "dawn/prog/process_template.hxx"

namespace dawn
{
/**
 * @brief Policy for running average calculation.
 *
 * Implements the policy pattern for running average computation.
 */

struct StatsOpAvg
{
  template<typename T>
  static inline void apply(size_t idx, io_ddata_t *ioData, io_ddata_t *outputData, bool &update)
  {
    outputData->get<T>(idx) += ioData->get<T>(idx);
    outputData->get<T>(idx) /= 2;
    update = true;
  }
};

/**
 * @brief Running average statistics Program.
 *
 * Calculates the running (exponential moving) average of values from a source
 * I/O.
 */

class CProgStatsAvg : public CProgProcessTemplate<StatsOpAvg>
{
public:
  explicit CProgStatsAvg(CDescObject &desc)
    : CProgProcessTemplate<StatsOpAvg>(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "avg";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_STATS_AVG, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_STATS_AVG,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgStatsAvg::cfgId(false, size, PROG_STATS_CFG_IOBIND);
  }
};
} // Namespace dawn
