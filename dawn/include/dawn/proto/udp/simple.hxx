// dawn/include/dawn/proto/udp/simple.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>
#include <mutex>

#include <netinet/in.h>
#include <poll.h>

#include "dawn/common/thread.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/proto/simplebase.hxx"

namespace dawn
{
/**
 * @brief Simple binary UDP protocol for device communication.
 *
 * CProtoUdp implements a lightweight, request-response UDP protocol for
 * communicating with embedded data acquisition devices.
 */

class CProtoUdp
  : public CProtoSimpleBase
  , protected CThreadedObject
{
public:
  enum
  {
    PROTO_UDP_CFG_FIRST = 0,
    PROTO_UDP_CFG_IOBIND = 1,
    PROTO_UDP_CFG_PORT = 2,
    PROTO_UDP_CFG_BIND_ADDR = 3,
    PROTO_UDP_CFG_LAST = 31
  };

  typedef SProtoSimpleIOBind SProtoUdpIOBind;

  explicit CProtoUdp(CDescObject &desc)
    : CProtoSimpleBase(desc)
    , port(CONFIG_DAWN_PROTO_UDP_PORT)
    , fd(-1)
    , sender()
  {
  }

  ~CProtoUdp() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "udp";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;
  bool hasThread() const override;

  constexpr static SObjectId::ObjectId objectId(uint16_t id)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_UDP, SObjectId::DTYPE_ANY, 0, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_UDP, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProtoUdp::cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_UDP_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPort()
  {
    return CProtoUdp::cfgId(true, SObjectId::DTYPE_UINT16, 1, PROTO_UDP_CFG_PORT);
  }

private:
  uint16_t port;             ///< Local UDP port.
  int fd;                    ///< File descriptor for UDP socket.
  struct sockaddr_in sender; ///< Last sender address for replies.
  std::mutex senderMutex;    ///< Mutex for sender address protection.

  int sendFrame(uint8_t cmd, const uint8_t *payload, size_t len) override;

  int configureDesc(const CDescObject &desc);
  int udpInit();
  void thread();
  int pollBefore(struct pollfd *pfds, nfds_t nfds);
  void pollAfter(int ret);
  int pollOnReady(struct pollfd *pfds, nfds_t nfds, int pollRet);

  static int cbPollBefore(void *priv, struct pollfd *pfds, nfds_t nfds);
  static void cbPollAfter(void *priv, struct pollfd *pfds, nfds_t nfds, int ret);
  static int cbPollOnReady(void *priv, struct pollfd *pfds, nfds_t nfds, int pollRet);
};

} // Namespace dawn
