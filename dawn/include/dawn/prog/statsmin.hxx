// dawn/include/dawn/prog/statsmin.hxx
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
 * @brief Policy for minimum value tracking.
 *
 * Template specialization allows type-safe comparison:
 * - int32_t: Signed comparison
 * - uint32_t: Unsigned comparison
 * - float: IEEE floating-point comparison.
 */

struct StatsOpMin
{
  template<typename T>
  static inline void apply(size_t idx, io_ddata_t *ioData, io_ddata_t *outputData, bool &update)
  {
    if (ioData->get<T>(idx) < outputData->get<T>(idx))
      {
        outputData->get<T>(idx) = ioData->get<T>(idx);
        update = true;
      }
  }
};

/**
 * @brief Minimum value tracking statistics Program.
 *
 * Tracks the minimum value from a source I/O over time.
 */

class CProgStatsMin : public CProgProcessTemplate<StatsOpMin>
{
public:
  explicit CProgStatsMin(CDescObject &desc)
    : CProgProcessTemplate<StatsOpMin>(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "min";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_STATS_MIN, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_STATS_MIN,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgStatsMin::cfgId(false, size, PROG_STATS_CFG_IOBIND);
  }
};
} // Namespace dawn
