// dawn/include/dawn/proto/serial/simple.hxx
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
 * @brief Simple binary serial protocol for device communication.
 *
 * CProtoSerial implements a lightweight, request-response serial protocol for
 * communicating with embedded data acquisition devices.
 */

class CProtoSerial
  : public CProtoSimpleBase
  , protected CThreadedObject
{
public:
  enum
  {
    PROTO_SERIAL_CFG_FIRST = 0,
    PROTO_SERIAL_CFG_IOBIND = 1,
    PROTO_SERIAL_CFG_PATH = 2,
    PROTO_SERIAL_CFG_BAUD = 3,
    PROTO_SERIAL_CFG_LAST = 31
  } typedef EProtoSerialCfg;

  typedef SProtoSimpleIOBind SProtoSerialIOBind;

  explicit CProtoSerial(CDescObject &desc)
    : CProtoSimpleBase(desc)
    , path(CONFIG_DAWN_PROTO_SERIAL_PATH)
    , baud(CONFIG_DAWN_PROTO_SERIAL_BAUD)
    , fd(-1)
    , parserPos(0)
    , parserLen(0)
    , parserState(0)
  {
  }

  ~CProtoSerial() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "serial";
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
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_SERIAL, SObjectId::DTYPE_ANY, 0, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_SERIAL, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProtoSerial::cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_SERIAL_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPath(uint16_t size)
  {
    return CProtoSerial::cfgId(true, SObjectId::DTYPE_CHAR, size, PROTO_SERIAL_CFG_PATH);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdBaud()
  {
    return CProtoSerial::cfgId(true, SObjectId::DTYPE_UINT32, 1, PROTO_SERIAL_CFG_BAUD);
  }

private:
  const char *path;   ///< Serial device path string.
  uint32_t baud;      ///< Baud rate.
  int fd;             ///< File descriptor for serial port.
  size_t parserPos;   ///< Parser cursor into rxbuffer.
  uint16_t parserLen; ///< Payload length decoded from current frame.
  int parserState;    ///< Frame parser state machine value.

  int sendFrame(uint8_t cmd, const uint8_t *payload, size_t len) override;

  int configureDesc(const CDescObject &desc);
  int serialInit();
  void thread();
  int pollBefore(struct pollfd *pfds, nfds_t nfds);
  void pollAfter(int ret);
  int pollOnReady(struct pollfd *pfds, nfds_t nfds, int pollRet);

  static int cbPollBefore(void *priv, struct pollfd *pfds, nfds_t nfds);
  static void cbPollAfter(void *priv, struct pollfd *pfds, nfds_t nfds, int ret);
  static int cbPollOnReady(void *priv, struct pollfd *pfds, nfds_t nfds, int pollRet);
};

} // Namespace dawn
