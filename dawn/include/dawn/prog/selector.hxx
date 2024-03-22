// dawn/include/dawn/prog/selector.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>
#include <inttypes.h>
#include <vector>

#include "dawn/io/ddata.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;

/**
 * @brief Data selector: routes one of N data inputs to a target IO based on
 * the value of a control input.
 *
 * When the control input changes, the program copies the selected data
 * input's current value to the target via the target's setData interface.
 * Indexing is zero-based; out-of-range control values are ignored.
 */
class CProgSelector : public CProgCommon
{
public:
  enum
  {
    PROG_SELECTOR_CFG_FIRST = 0,
    PROG_SELECTOR_CFG_CONTROL = 1,
    PROG_SELECTOR_CFG_DATA = 2,
    PROG_SELECTOR_CFG_TARGET = 3,
    PROG_SELECTOR_CFG_LAST = 31
  };

  explicit CProgSelector(CDescObject &desc);

  ~CProgSelector() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "selector";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_SELECTOR, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_SELECTOR,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdControl()
  {
    return CProgSelector::cfgId(false, 1, PROG_SELECTOR_CFG_CONTROL);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdData(uint16_t size)
  {
    return CProgSelector::cfgId(false, size, PROG_SELECTOR_CFG_DATA);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdTarget()
  {
    return CProgSelector::cfgId(false, 1, PROG_SELECTOR_CFG_TARGET);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t = 0)
  {
    return CProgSelector::cfgId(false, 0, 0);
  }

private:
  struct SDataBind
  {
    CProgSelector *owner;
    size_t index;
  };

  CIOCommon *ctrlIo;                        ///< Control IO.
  std::vector<CIOCommon *> dataIos;         ///< Data source IOs.
  std::vector<SDataBind> dataBinds;         ///< Per-data notifier bindings.
  std::vector<SObjectId::ObjectId> dataIds; ///< Data IO ObjectIds.
  CIOCommon *target;                        ///< Target IO.
  SObjectId::ObjectId ctrlId;               ///< Control IO ObjectId.
  SObjectId::ObjectId targetId;             ///< Target IO ObjectId.
  io_ddata_t *iodata;                       ///< Buffer for setData / getData.
  uint32_t currentIndex;                    ///< Currently selected data index.
  bool active;                              ///< Activation flag.
  bool registered;                          ///< Whether IO notifiers are registered.

  static int ctrlNotifierCb(void *priv, io_ddata_t *data);
  static int dataNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  void applyRoute(uint32_t index);
};
} // Namespace dawn
