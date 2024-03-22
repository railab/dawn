// dawn/include/dawn/prog/statscount.hxx
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
 * @brief Policy for sample counting.
 *
 * Counts the number of samples received from source I/O.
 */

struct StatsOpCount
{
  template<typename T>
  static inline void apply(size_t idx, io_ddata_t *ioData, io_ddata_t *outputData, bool &update)
  {
    (void)ioData;
    outputData->get<T>(idx) += 1;
    update = true;
  }
};

/**
 * @brief Sample counting statistics Program.
 *
 * Counts the number of samples received from a source I/O over time.
 */

class CProgStatsCount : public CProgProcessTemplate<StatsOpCount>
{
public:
  explicit CProgStatsCount(CDescObject &desc)
    : CProgProcessTemplate<StatsOpCount>(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "count";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_STATS_COUNT, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_STATS_COUNT,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgStatsCount::cfgId(false, size, PROG_STATS_CFG_IOBIND);
  }
};
} // Namespace dawn
