// dawn/include/dawn/proto/nxscope/nxscope.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/thread.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/proto/common.hxx"
#include "logging/nxscope/nxscope.h"
#include <cstddef>
#include <mutex>
#include <semaphore.h>

// Recv is non blocking and uses usleep.
// TODO: blocking recv doesn't work for some reason - investigate per
//       transport (UDP/serial). Until resolved, NXSCOPE_RECV_NONBLOCK is
//       required to keep threadRecv() responsive to stop requests.

#define NXSCOPE_RECV_NONBLOCK (1)

namespace dawn
{
// Forward declaration

class IIOCommon;
class CIOCommon;
struct io_ddata_t;

/**
 * @brief Real-time data visualization protocol (base class).
 *
 * CProtoNxscope is the base class for NXScope-based real-time data
 * visualization and monitoring protocols.
 */

class CProtoNxscope : public CProtoCommon
{
public:
  struct
  {
    SObjectId::UObjectId objid; ///< I/O object ID to visualize.
  } typedef SProtoNxscopeIOBind;

  struct
  {
    const char name[12]; ///< Channel name (12 bytes max, 3x 4B words).
  } typedef SProtoNxscopeNames;

  struct
  {
    SProtoNxscopeIOBind bind; ///< I/O object binding.
    SProtoNxscopeNames name;  ///< Channel name.
  } typedef SProtoNxscopeIOBind2;

  explicit CProtoNxscope(CDescObject &desc)
    : CProtoCommon(desc)
    , nxsIntf()
    , threadRecvMember()
#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
    , threadSampleMember()
#endif
    , nxs()
    , nxsCfg()
    , nxsProto()
    , nxsCbs()
  {
  }

  ~CProtoNxscope() override;

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;
  bool hasThread() const override;

protected:
  struct nxscope_intf_s nxsIntf; ///< NXScope interface structure (from logging library).

  int allocObject(SProtoNxscopeIOBind *alloc);
  int allocNames(size_t j, SProtoNxscopeNames *names);
  int configureNxscope();

  virtual int initPriv() = 0;
  virtual int deinitPriv() = 0;
  virtual int startPriv() = 0;
  virtual int stopPriv() = 0;

private:
#ifdef CONFIG_DAWN_IO_NOTIFY
  typedef int (
    *nxscope_put_cb_t)(struct nxscope_s *nxs, uint8_t chan, void *data, uint8_t dim, size_t batch);
#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
  struct proto_nxscope_iochan_s;

  typedef int (*nxscope_sample_cb_t)(CProtoNxscope *obj, struct proto_nxscope_iochan_s *iochan);
#endif

  struct proto_nxscope_iochan_s
  {
    uint8_t chan;                  ///< NXScope channel ID.
    uint8_t dim;                   ///< Number of data items in one sample.
    bool stream;                   ///< True when channel participates in stream path.
    CIOCommon *io;                 ///< Pointer to I/O object.
    CProtoNxscope *obj;            ///< Reference to owner handler.
    io_ddata_t *setData;           ///< Dynamic buffer used for set requests.
    io_ddata_t *getData = nullptr; ///< Lazily allocated get request buffer.

#ifdef CONFIG_DAWN_IO_NOTIFY
    nxscope_put_cb_t put;          ///< Bound nxscope_put_v* callback.
#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
    nxscope_sample_cb_t sample; ///< Bound typed get+put callback.
    io_ddata_t *data;           ///< Dynamic sample buffer bound to this channel.
#endif
  } typedef SProtoNxscopeIochan;

  enum
  {
    NXSCOPE_USER_SET_IO = NXSCOPE_HDRID_USER,
    NXSCOPE_USER_SET_IO_SEEK = NXSCOPE_HDRID_USER + 1,
    NXSCOPE_USER_GET_IO = NXSCOPE_HDRID_USER + 2,
    NXSCOPE_USER_GET_IO_SEEK = NXSCOPE_HDRID_USER + 3
  };

  CThreadedObject threadRecvMember;   ///< Receive thread for client commands.

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
  CThreadedObject threadSampleMember; ///< Sample thread for periodic data collection.
#endif

  struct nxscope_s nxs;
  struct nxscope_cfg_s nxsCfg;
  struct nxscope_proto_s nxsProto;
  struct nxscope_callbacks_s nxsCbs;

  /// Serializes stream buffer producers against the stream flush. Taken
  /// once per batch, so it can replace the per-sample nxscope put lock
  /// (CONFIG_LOGGING_NXSCOPE_DISABLE_PUTLOCK).
  std::mutex streamLock;

  /// Posted by the notifier when a batch has been put into the stream
  /// buffer. threadRecv() waits on it so it flushes at the data rate
  /// instead of on a fixed timer (which is limited to one flush per
  /// system tick).
  sem_t streamSem;

  std::vector<const SProtoNxscopeIOBind *> vchannels; ///< Channel I/O bindings (from descriptor).
  std::vector<const SProtoNxscopeNames *> vnames;     ///< Channel names (from descriptor).
  std::vector<SProtoNxscopeIochan> vio;               ///< Runtime I/O-to-channel mappings.
  bool hasStreamChannels = false;                     ///< True when stream-capable channels

  SProtoNxscopeIochan *findIochan(SObjectId::ObjectId objid);

  int handleUserCommand(uint8_t id, uint8_t *buff);
  int userSetIO(uint8_t *buff);
  int userSetIOSeek(uint8_t *buff);
  int userGetIO(uint8_t *buff);
  int userGetIOSeek(uint8_t *buff);
  int sendAck(int ret);
  int sendUserData(SObjectId::ObjectId objid, const void *data, uint16_t size);

  static int userIdCb(void *priv, uint8_t id, uint8_t *buff);

#ifdef CONFIG_DAWN_IO_NOTIFY
  static int ioNotifierCb(void *priv, io_ddata_t *data);
#endif

  uint8_t getChannelDim(const CIOCommon &io);
  uint8_t getChannelDtype(const CIOCommon &io);
  int nxscopeChannelsCreate();
  int bindChannelCallbacks(SProtoNxscopeIochan &iochan, uint8_t dtype);

#ifdef CONFIG_DAWN_IO_NOTIFY
  static int putInt16(struct nxscope_s *nxs, uint8_t chan, void *data, uint8_t dim, size_t batch);
  static int putInt32(struct nxscope_s *nxs, uint8_t chan, void *data, uint8_t dim, size_t batch);
  static int putUint32(struct nxscope_s *nxs, uint8_t chan, void *data, uint8_t dim, size_t batch);
  static int putUint64(struct nxscope_s *nxs, uint8_t chan, void *data, uint8_t dim, size_t batch);
  static int putFloat(struct nxscope_s *nxs, uint8_t chan, void *data, uint8_t dim, size_t batch);
#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
  void getAndPut(SProtoNxscopeIochan *iochan);

  static int sampleInt16(CProtoNxscope *obj, SProtoNxscopeIochan *iochan);
  static int sampleInt32(CProtoNxscope *obj, SProtoNxscopeIochan *iochan);
  static int sampleUint32(CProtoNxscope *obj, SProtoNxscopeIochan *iochan);
  static int sampleUint64(CProtoNxscope *obj, SProtoNxscopeIochan *iochan);
  static int sampleFloat(CProtoNxscope *obj, SProtoNxscopeIochan *iochan);
#endif

  void threadRecv();

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
  void threadSample();
#endif
};
} // Namespace dawn
