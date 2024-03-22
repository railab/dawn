// dawn/include/dawn/prog/statsmax.hxx
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
 * @brief Policy for maximum value tracking.
 *
 * Implements the policy pattern for maximum value accumulation.
 */

struct StatsOpMax
{
  template<typename T>
  static inline void apply(size_t idx, io_ddata_t *ioData, io_ddata_t *outputData, bool &update)
  {
    if (ioData->get<T>(idx) > outputData->get<T>(idx))
      {
        outputData->get<T>(idx) = ioData->get<T>(idx);
        update = true;
      }
  }
};

/**
 * @brief Maximum value tracking statistics Program.
 *
 * Tracks the maximum value from a source I/O over time.
 */

class CProgStatsMax : public CProgProcessTemplate<StatsOpMax>
{
public:
  explicit CProgStatsMax(CDescObject &desc)
    : CProgProcessTemplate<StatsOpMax>(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "max";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_STATS_MAX, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_STATS_MAX,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgStatsMax::cfgId(false, size, PROG_STATS_CFG_IOBIND);
  }
};
} // Namespace dawn
