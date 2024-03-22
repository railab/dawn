// dawn/include/dawn/prog/sampling.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vector>

#include "dawn/common/thread.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;
class io_ddata_t;

/**
 * @brief Periodic data sampling and buffering Program.
 *
 * Periodically acquires samples from source I/O objects and writes them to
 * generic target I/O objects.
 */

class CProgSampling : public CProgCommon
{
public:
  constexpr static uint32_t INTERVAL_DEFAULT = CONFIG_DAWN_PROG_SAMPLING_INTERVAL;

  enum
  {
    PROG_SAMPLING_CFG_FIRST = 0,
    PROG_SAMPLING_CFG_IOBIND = 1,   ///< I/O binding configuration.
    PROG_SAMPLING_CFG_INTERVAL = 2, ///< Sampling interval (microseconds).
    PROG_SAMPLING_CFG_LAST = 31
  };

  CProgSampling(CDescObject &desc)
    : CProgCommon(desc)
    , interval(INTERVAL_DEFAULT)
  {
  }

  ~CProgSampling() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "sampling";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_SAMPLING, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_SAMPLING,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProgSampling::cfgId(false, size, PROG_SAMPLING_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOInterval()
  {
    return CProgSampling::cfgId(true, 1, PROG_SAMPLING_CFG_INTERVAL);
  }

private:
  struct SSamplingBind
  {
    SObjectId::ObjectId srcId;
    SObjectId::ObjectId targetId;
    class CIOCommon *src;
    class CIOCommon *target;
    io_ddata_t *iodata;
  };

  std::vector<SSamplingBind *> binds; ///< Sampling bindings table.
  uint32_t interval;                  ///< Sampling interval in microseconds.
  CThreadedObject threadCtl;          ///< Thread management object.

  int configureDesc(const CDescObject &desc);
  int allocObject(SObjectId::ObjectId srcId, SObjectId::ObjectId targetId);
  void thread();
};
} // Namespace dawn
