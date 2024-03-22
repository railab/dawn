// dawn/include/dawn/prog/buffer.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;
class CIOVirt;
class io_ddata_t;

/**
 * @brief Notify-driven history buffer Program.
 *
 * Captures source IO samples into program-owned RAM and exposes selected
 * history samples through generic virtIO channels.
 */

class CProgBuffer : public CProgCommon
{
public:
  constexpr static uint32_t DEPTH_DEFAULT = 16;

  enum
  {
    PROG_BUFFER_CFG_FIRST = 0,
    PROG_BUFFER_CFG_IOBIND = 1,     ///< I/O binding configuration.
    PROG_BUFFER_CFG_DEPTH = 2,      ///< Buffer depth.
    PROG_BUFFER_CFG_FLAGS = 3,      ///< Config flags.
    PROG_BUFFER_CFG_CHUNK_SIZE = 4, ///< Output samples per read.
    PROG_BUFFER_CFG_LAST = 31
  };

  enum
  {
    FLAG_AUTO_START = (1u << 0),
    FLAG_MODE_ONESHOT = (1u << 1),
    FLAG_KEEP_DATA_ON_STOP = (1u << 2),
  } typedef EProgBufferFlags;

  /** @brief One buffer binding payload. */

  struct
  {
    SObjectId::UObjectId src;  ///< Source IO providing samples via notify.
    SObjectId::UObjectId out;  ///< Selected history sample to readers.
    SObjectId::UObjectId sel;  ///< History offset selector (0 = newest).
    SObjectId::UObjectId stat; ///< Buffer status words (see STAT_*).
  } typedef SProgBufferIOBind;

  explicit CProgBuffer(CDescObject &desc)
    : CProgCommon(desc)
    , bind(nullptr)
    , depth(DEPTH_DEFAULT)
    , chunkSize(1)
    , flags(FLAG_AUTO_START)
  {
  }

  ~CProgBuffer() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "buffer";
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
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_BUFFER, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_BUFFER, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProgBuffer::cfgId(false, size, PROG_BUFFER_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdDepth()
  {
    return CProgBuffer::cfgId(false, 1, PROG_BUFFER_CFG_DEPTH);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdFlags()
  {
    return CProgBuffer::cfgId(false, 1, PROG_BUFFER_CFG_FLAGS);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdChunkSize()
  {
    return CProgBuffer::cfgId(false, 1, PROG_BUFFER_CFG_CHUNK_SIZE);
  }

private:
  /** @brief Word indices into the status virtIO buffer (wire contract). */

  enum
  {
    STAT_COUNT = 0,           ///< Number of valid samples in ring.
    STAT_DEPTH = 1,           ///< Configured ring depth.
    STAT_HEAD = 2,            ///< Next write index into ring.
    STAT_OVERFLOW = 3,        ///< Samples overwritten in continuous mode.
    STAT_SNAPSHOT_SEQ = 4,    ///< Snapshot sequence counter.
    STAT_RUNTIME_FLAGS = 5,   ///< Runtime flag bitmap (see RUNTIME_*).
    STAT_SELECTED_OFFSET = 6, ///< Current selector offset.
    STAT_RESERVED = 7,
    STAT_WORDS = 8            ///< Total number of stat words.
  };

  /** @brief Bit definitions inside STAT_RUNTIME_FLAGS (wire contract). */

  enum
  {
    RUNTIME_RUNNING = (1u << 0),        ///< Program is started.
    RUNTIME_CAPTURE_ACTIVE = (1u << 1), ///< Capture path is enabled.
    RUNTIME_FULL = (1u << 2),           ///< Ring is full (count == depth).
    RUNTIME_ERROR_RANGE = (1u << 3),    ///< Selector out of range.
  };

  /** @brief Per-program runtime state. */

  struct SBufferBind
  {
    SObjectId::ObjectId srcId;  // source IO ID
    SObjectId::ObjectId outId;  // selected sample IO ID
    SObjectId::ObjectId selId;  // selector IO ID
    SObjectId::ObjectId statId; // status IO ID
    CIOCommon *src;             // source IO
    CIOVirt *out;               // selected history sample/chunk
    CIOVirt *sel;               // history offset selector.
    CIOVirt *stat;              // status words.
    io_ddata_t *ring;           // ring for captured samples
    io_ddata_t *outData;        // scratch buffer for out
    io_ddata_t *selData;        // scratch buffer for sel
    io_ddata_t *statData;       // scratch buffer for stat
    uint32_t head;              // next write index into ring
    uint32_t count;             // samples in the ring.
    uint32_t overflow;          // overflow counter
    uint32_t selectedOffset;    // selector offset last applied
    uint32_t snapshotSeq;       // snapshot sequence counter
    bool captureActive;         // capture path enabled
  };

  SBufferBind *bind;
  uint32_t depth;
  uint32_t chunkSize;
  uint32_t flags;

  int configureDesc(const CDescObject &desc);
  int allocBind(SObjectId::ObjectId src,
                SObjectId::ObjectId out,
                SObjectId::ObjectId sel,
                SObjectId::ObjectId stat);
  int validateBind();
  int allocateBind();
  void clearBind();
  int captureBind(io_ddata_t *data);
  int updateSelected();
  int updateStat();

  void cmdReset();
  void cmdStartCapture();
  void cmdStopCapture();

  static uint32_t resolveRingIndex(const SBufferBind *bind, uint32_t sel, uint32_t depth);

  static int srcNotifyCb(void *priv, io_ddata_t *data);
  static void outGetCb(CIOVirt *io, void *priv);
  static void selSetCb(CIOVirt *io, void *priv);
  static void statGetCb(CIOVirt *io, void *priv);
};
} // Namespace dawn
