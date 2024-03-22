// dawn/include/dawn/prog/gateway.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vector>

#include "dawn/io/virt.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
// Forward declaration

class CIOVirt;
class io_ddata_t;

/**
 * @brief Protocol-to-protocol IO gateway Program.
 *
 * Bridges pairs of Virtual I/O objects so that different protocols can share
 * IO state. Each binding connects io1 and io2 with per-binding permission
 * flags. Write propagation (notify) and on-demand fetch are both supported.
 */

class CProgGateway : public CProgCommon
{
public:
  enum
  {
    PROG_GATEWAY_CFG_FIRST = 0,
    PROG_GATEWAY_CFG_IOBIND = 1,
    PROG_GATEWAY_CFG_LAST = 31
  };

  enum
  {
    FLAG_IO1_READ = (1 << 0),  ///< io1 get callback: fetches from io2.
    FLAG_IO1_WRITE = (1 << 1), ///< io1 set callback: propagates to io2.
    FLAG_IO2_READ = (1 << 2),  ///< io2 get callback: fetches from io1.
    FLAG_IO2_WRITE = (1 << 3), ///< io2 set callback: propagates to io1.
  } typedef EProgGatewayFlags;

  /**
   * @brief Descriptor payload for one binding (4 uint32_t words).
   *
   * One or more of these follow a single cfgIdIOBind() config header.
   */

  struct
  {
    SObjectId::UObjectId io1; ///< First Virtual IO object ID.
    SObjectId::UObjectId io2; ///< Second Virtual IO object ID.
    uint32_t flags;           ///< EProgGatewayFlags bitmask.
    uint32_t dim;             ///< VirtIO data dimension (number of elements).
  } typedef SProgGatewayIOBind;

  explicit CProgGateway(CDescObject &desc)
    : CProgCommon(desc)
  {
  }

  ~CProgGateway() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "gateway";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_GATEWAY, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_GATEWAY, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProgGateway::cfgId(false, size, PROG_GATEWAY_CFG_IOBIND);
  }

private:
  struct SGatewayBind
  {
    SProgGatewayIOBind cfg;
    CIOVirt *vio1;
    CIOVirt *vio2;
    io_ddata_t *iodata;
  };

  std::vector<SGatewayBind *> binds; ///< Allocated during configure, completed in init().

  int configureDesc(const CDescObject &desc);
  int allocObject(const SProgGatewayIOBind *cfg);

  static void io1SetCb(CIOVirt *io, void *priv);
  static void io2SetCb(CIOVirt *io, void *priv);
  static void io1GetCb(CIOVirt *io, void *priv);
  static void io2GetCb(CIOVirt *io, void *priv);
};
} // Namespace dawn
