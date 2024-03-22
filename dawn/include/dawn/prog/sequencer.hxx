// dawn/include/dawn/prog/sequencer.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <mutex>
#include <vector>

#include "dawn/common/thread.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;
class io_ddata_t;

/**
 * @brief Periodic state sequencer Program.
 *
 * Applies a configured value sequence to one or more output IO targets.
 * Each state stores a value and dwell time in microseconds.
 */

class CProgSequencer : public CProgCommon
{
public:
  enum
  {
    PROG_SEQUENCER_CFG_FIRST = 0,
    PROG_SEQUENCER_CFG_TARGET = 1,
    PROG_SEQUENCER_CFG_STATE = 2,
    PROG_SEQUENCER_CFG_START = 3,
    PROG_SEQUENCER_CFG_LAST = 31
  };

  explicit CProgSequencer(CDescObject &desc)
    : CProgCommon(desc)
    , iodata(nullptr)
    , startIndex(0)
    , currentIndex(0)
    , targetSize(0)
    , targetDtype(SObjectId::DTYPE_ANY)
  {
  }

  ~CProgSequencer() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "sequencer";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;
  bool hasThread() const override;
  int trigger(uint8_t cmd) override;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_SEQUENCER, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_SEQUENCER,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdTargets(uint16_t size)
  {
    return CProgSequencer::cfgId(false, size, PROG_SEQUENCER_CFG_TARGET);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdStates(uint16_t size)
  {
    return CProgSequencer::cfgId(true, size, PROG_SEQUENCER_CFG_STATE);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdStartIndex()
  {
    return CProgSequencer::cfgId(true, 1, PROG_SEQUENCER_CFG_START);
  }

private:
  int onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len) override;

  struct SState
  {
    uint32_t value;   ///< Output value written to targets (raw 32-bit payload).
    uint32_t dwellUs; ///< Dwell duration for this state in microseconds.
  };

  int configureDesc(const CDescObject &desc);
  int allocObject(SObjectId::ObjectId targetId);
  int reloadRuntimeConfig(bool resetCurrent,
                          SObjectCfg::ObjectCfgId overrideCfg = 0,
                          const uint32_t *overrideData = nullptr,
                          size_t overrideLen = 0);
  int applyState(size_t index);
  void thread();

  std::vector<SObjectId::ObjectId> targetIds; ///< Output target IDs read from descriptor.
  std::vector<CIOCommon *> targets;           ///< Resolved output target IO objects.
  std::vector<SState> states;                 ///< Sequence state table.
  io_ddata_t *iodata;        ///< Reusable output buffer used for target setData calls.
  CThreadedObject threadCtl; ///< Worker thread controller.
  std::mutex stateLock;      ///< Mutex protecting current state index access.
  size_t startIndex;         ///< Initial state index for start/reset actions.
  size_t currentIndex;       ///< Currently active state index.
  size_t targetSize;         ///< Target payload size in bytes for one scalar value.
  uint8_t targetDtype;       ///< Target data type for io_ddata allocation.
};
} // Namespace dawn
