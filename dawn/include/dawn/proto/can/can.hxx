// dawn/include/dawn/proto/can/can.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <poll.h>
#include <vector>

#include "dawn/common/thread.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/proto/common.hxx"
#include "dawn/proto/can/isotp.hxx"

#if defined(CONFIG_DAWN_PROTO_CAN_SEG) || defined(CONFIG_DAWN_PROTO_CAN_SINGLE_ID)
#  define DAWN_PROTO_HAS_ISOTP
#endif

namespace dawn
{
// Forward declaration

struct io_ddata_t;

/**
 * @brief CAN bus protocol for industrial automation.
 *
 * CProtoCan implements a CAN-based protocol for real-time I/O communication on
 * CAN bus networks.
 */

class CProtoCan
  : public CProtoCommon
  , protected CThreadedObject
{
public:
  enum
  {
    PROTO_CAN_CFG_FIRST = 0,  ///< Reserved
    PROTO_CAN_CFG_IOBIND = 1, ///< I/O object binding
    PROTO_CAN_CFG_DEVNO = 2,  ///< CAN device number (0=can0)
    PROTO_CAN_CFG_NODEID = 3, ///< CAN node ID base address
    PROTO_CAN_CFG_LAST = 31   ///< Reserved
  };

  enum
  {
    CAN_TYPE_PUSH = 0,          ///< device autonomously sends data
    CAN_TYPE_READ = 1,          ///< request/response with RTR frame
    CAN_TYPE_WRITE = 2,         ///< master sends command
    CAN_TYPE_INDEXED_READ = 3,  ///< indexed request/response
    CAN_TYPE_INDEXED_WRITE = 4, ///< indexed request/response
    CAN_TYPE_READ_SEG = 5,      ///< segmented response, requested by data frame
    CAN_TYPE_WRITE_SEG = 6,     ///< segmented request
  };

  struct
  {
    uint32_t type;    ///< Access type (CAN_TYPE_*)
    uint32_t start;   ///< Start CAN address offset
    uint32_t size;    ///< Number of I/O objects in group
    uint32_t objid[]; ///< Array of I/O object IDs
  } typedef SProtoCanIOBind;

  explicit CProtoCan(CDescObject &desc)
    : CProtoCommon(desc)
    , devno(getPriv())
    , nodeid(CONFIG_DAWN_PROTO_CAN_NODEID)
    , fd(-1)
    , initialized(false)
  {
  }

  ~CProtoCan() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "can";
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
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_CAN, SObjectId::DTYPE_ANY, 0, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_CAN, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProtoCan::cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_CAN_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdDevno()
  {
    return CProtoCan::cfgId(true, SObjectId::DTYPE_UINT32, 1, PROTO_CAN_CFG_DEVNO);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdNodeid()
  {
    return CProtoCan::cfgId(true, SObjectId::DTYPE_UINT32, 1, PROTO_CAN_CFG_NODEID);
  }

private:
  struct
  {
    CProtoCan *proto;     ///< Parent protocol instance
    CIOCommon *io;        ///< Reference to I/O object
    uint16_t canid;       ///< CAN ID for this I/O
    io_ddata_t *iodata;   ///< Runtime I/O data pointer
    size_t data_len;      ///< Cached data length in bytes
#ifdef DAWN_PROTO_HAS_ISOTP
    CIsoTp::State *isotp; ///< Optional ISO-TP transfer state
#endif
  } typedef SCanIoData;

#ifdef DAWN_PROTO_HAS_ISOTP
  struct
  {
    CIsoTp::State isotp; ///< Write-all transfer state
    size_t io_idx;       ///< Current IO index for write-all
    size_t io_off;       ///< Current IO offset for write-all
  } typedef SProtoCanWriteAll;
#endif

  struct
  {
    const SProtoCanIOBind *cfg;   ///< Bind configuration
    size_t ios;                   ///< Number of I/Os in group
    SCanIoData *io;               ///< Array of I/O data for each object
#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
    SProtoCanWriteAll *write_all; ///< Optional write-all state
#endif
  } typedef SProtoCanRegs;

  char path[PATH_MAX] = {};              // CAN device path string (e.g. "/dev/can0")
  uint8_t devno;                         // CAN device number (0 = can0, 1 = can1, etc.)
  uint16_t nodeid;                       // CAN node ID base address for I/O mapping.
  int fd;                                // File descriptor for CAN socket.
  bool initialized;                      // Initialization completion flag

  std::vector<SProtoCanIOBind *> valloc; // Vector of I/O bindings (from descriptor).
  std::vector<SProtoCanRegs *> vregs;    // Vector of register groups.

  int configureDesc(const CDescObject &desc);
  void allocObject(SProtoCanIOBind *alloc);
  void thread();
  int createRegs();
  int validateCanIo(const SProtoCanIOBind *v, CIOCommon *io, SCanIoData *canio);
  int createRegsForGroup(SProtoCanIOBind *v);
  int destroyRegs();
  int canInitialize();
  int msgRecv(const dawn::porting::canmsg_s &msg);
#ifdef DAWN_PROTO_HAS_ISOTP
  int sendSegmented(uint16_t canid,
                    const uint8_t *data,
                    size_t len,
                    bool with_index,
                    uint8_t index);
  int sendSegmentedRead(SCanIoData *canio,
                        size_t len,
                        bool with_index,
                        uint8_t index,
                        size_t offset = 0);
#endif
  int sendIoResponse(SCanIoData *canio, uint8_t index, bool segmented);
#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
  int handleSingleIdGroup(const dawn::porting::canmsg_s &msg, SProtoCanRegs *v);
  int handleSingleIdReadAll(const dawn::porting::canmsg_s &msg,
                            SProtoCanRegs *v,
                            bool ignore_len = false);
  int handleSingleIdWriteAll(const dawn::porting::canmsg_s &msg,
                             SProtoCanRegs *v,
                             uint8_t seg_no,
                             bool seg_last);
  int handleSingleIdIndex(const dawn::porting::canmsg_s &msg,
                          SProtoCanRegs *v,
                          uint8_t index,
                          uint8_t seg_no,
                          bool seg_last);
#endif
  int handleLegacyGroup(const dawn::porting::canmsg_s &msg, SProtoCanRegs *v);

  static int beforePoll(void *priv, struct pollfd *pfds, nfds_t nfds);
  static void afterPoll(void *priv, struct pollfd *pfds, nfds_t nfds, int ret);
  static int onPollReady(void *priv, struct pollfd *pfds, nfds_t nfds, int pollRet);
  static int notifierCb(void *priv, io_ddata_t *data);
};

} // Namespace dawn
