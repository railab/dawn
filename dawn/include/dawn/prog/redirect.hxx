// dawn/include/dawn/prog/redirect.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vector>

#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;
class io_ddata_t;

/**
 * @brief Input-to-output routing Program.
 *
 * Routes samples from notify-capable source IOs to destination IOs.
 * This Program is notify-only. For fetch-based polling pipelines, combine
 * ``sampling`` with ``redirect`` instead of embedding a thread here.
 */

class CProgRedirect : public CProgCommon
{
public:
  enum
  {
    PROG_REDIRECT_CFG_FIRST = 0,
    PROG_REDIRECT_CFG_IOBIND = 1,
    PROG_REDIRECT_CFG_LAST = 31
  };

  /** @brief One redirect binding payload (2 uint32_t words). */

  struct
  {
    SObjectId::UObjectId src;
    SObjectId::UObjectId dst;
  } typedef SProgRedirectIOBind;

  explicit CProgRedirect(CDescObject &desc)
    : CProgCommon(desc)
  {
  }

  ~CProgRedirect() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "redirect";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_REDIRECT, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_REDIRECT,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProgRedirect::cfgId(false, size, PROG_REDIRECT_CFG_IOBIND);
  }

private:
  struct SRedirectBind
  {
    SProgRedirectIOBind cfg;
    CIOCommon *src;
    CIOCommon *dst;
    bool active;
  };

  std::vector<SRedirectBind *> binds;

  int configureDesc(const CDescObject &desc);
  int allocObject(const SProgRedirectIOBind *cfg);

#ifdef CONFIG_DAWN_IO_NOTIFY
  static int ioNotifierCb(void *priv, io_ddata_t *data);
#endif
};
} // Namespace dawn
