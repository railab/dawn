// dawn/include/dawn/prog/statssum.hxx
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
 * @brief Policy for sum accumulation.
 *
 * Implements the policy pattern for continuous sum calculation.
 */

struct StatsOpSum
{
  template<typename T>
  static inline void apply(size_t idx, io_ddata_t *ioData, io_ddata_t *outputData, bool &update)
  {
    outputData->get<T>(idx) += ioData->get<T>(idx);
    update = true;
  }
};

/**
 * @brief Sum accumulation statistics Program.
 *
 * Accumulates the sum of all values from a source I/O over time.
 */

class CProgStatsSum : public CProgProcessTemplate<StatsOpSum>
{
public:
  explicit CProgStatsSum(CDescObject &desc)
    : CProgProcessTemplate<StatsOpSum>(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "sum";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_STATS_SUM, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_STATS_SUM,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgStatsSum::cfgId(false, size, PROG_STATS_CFG_IOBIND);
  }
};
} // Namespace dawn
