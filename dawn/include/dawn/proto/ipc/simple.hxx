// dawn/include/dawn/proto/ipc/simple.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>

#include <poll.h>

#include "dawn/common/thread.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/proto/simplebase.hxx"

namespace dawn
{
/**
 * @brief Simple FIFO-based IPC transport for local Dawn communication.
 *
 * CProtoIpc exposes the Dawn simple binary protocol through two named pipes:
 * one FIFO for incoming commands and one FIFO for outgoing responses.
 * On NuttX these are pipe-driver paths in the pseudo-filesystem (for example
 * under /var/pipe), not regular files on a mounted filesystem like /tmp.
 */

class CProtoIpc
  : public CProtoSimpleBase
  , protected CThreadedObject
{
public:
  enum
  {
    PROTO_IPC_CFG_FIRST = 0,
    PROTO_IPC_CFG_IOBIND = 1,
    PROTO_IPC_CFG_RX_PATH = 2,
    PROTO_IPC_CFG_TX_PATH = 3,
    PROTO_IPC_CFG_LAST = 31
  } typedef EProtoIpcCfg;

  typedef SProtoSimpleIOBind SProtoIpcIOBind;

  explicit CProtoIpc(CDescObject &desc)
    : CProtoSimpleBase(desc)
    , rxPath(CONFIG_DAWN_PROTO_IPC_RX_PATH)
    , txPath(CONFIG_DAWN_PROTO_IPC_TX_PATH)
    , rxfd(-1)
    , txfd(-1)
    , parserPos(0)
    , parserLen(0)
    , parserState(0)
  {
  }

  ~CProtoIpc() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "ipc";
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
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_IPC, SObjectId::DTYPE_ANY, 0, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_IPC, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProtoIpc::cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_IPC_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdRxPath(uint16_t size)
  {
    return CProtoIpc::cfgId(true, SObjectId::DTYPE_CHAR, size, PROTO_IPC_CFG_RX_PATH);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdTxPath(uint16_t size)
  {
    return CProtoIpc::cfgId(true, SObjectId::DTYPE_CHAR, size, PROTO_IPC_CFG_TX_PATH);
  }

private:
  const char *rxPath; ///< FIFO path used for incoming commands.
  const char *txPath; ///< FIFO path used for outgoing responses.
  int rxfd;           ///< Receive FIFO file descriptor.
  int txfd;           ///< Transmit FIFO file descriptor.
  size_t parserPos;   ///< Parser cursor into rxbuffer.
  uint16_t parserLen; ///< Payload length decoded from current frame.
  int parserState;    ///< Frame parser state machine value.

  int sendFrame(uint8_t cmd, const uint8_t *payload, size_t len) override;

  int configureDesc(const CDescObject &desc);
  int ensureFifo(const char *path);
  int fifoInit();
  void thread();
  int pollBefore(struct pollfd *pfds, nfds_t nfds);
  void pollAfter(int ret);
  int pollOnReady(struct pollfd *pfds, nfds_t nfds, int pollRet);

  static int cbPollBefore(void *priv, struct pollfd *pfds, nfds_t nfds);
  static void cbPollAfter(void *priv, struct pollfd *pfds, nfds_t nfds, int ret);
  static int cbPollOnReady(void *priv, struct pollfd *pfds, nfds_t nfds, int pollRet);
};
} // Namespace dawn
