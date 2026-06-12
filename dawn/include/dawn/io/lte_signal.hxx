// dawn/include/dawn/io/lte_signal.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/common.hxx"
#include "dawn/io/timerfd.hxx"

namespace dawn
{
/**
 * @brief LTE signal-quality vector layout (indices into the data vector).
 */

enum
{
  DAWN_LTE_SIGNAL_RSRP = 0, ///< Reference Signal Received Power (dBm)
  DAWN_LTE_SIGNAL_RSRQ = 1, ///< Reference Signal Received Quality (dB)
  DAWN_LTE_SIGNAL_SINR = 2, ///< Signal to Interference + Noise Ratio (dB)
  DAWN_LTE_SIGNAL_BAND = 3, ///< Serving E-UTRA band number
  DAWN_LTE_SIGNAL_DIM = 4,  ///< Number of metrics in the vector
};

/**
 * @brief LTE modem signal-quality I/O (read-only, timer-driven).
 *
 * A single read snapshots the current modem metrics as one int16 vector
 * [RSRP, RSRQ, SINR, band] (signed dBm/dB) read via the target-independent
 * NuttX LTE API. A timerfd polls the modem every 'interval' microseconds and
 * emits a notification, so a 'vecsplit' program can fan the vector out into
 * the individual scalars (e.g. onto the LwM2M Connectivity Monitoring object).
 */

class CIOLteSignal
  : public CIOCommon
  , public CIOTimerfd
{
public:
  /** @brief Config item ids (cls = IO_CLASS_LTE_SIGNAL). */

  enum
  {
    LTE_SIGNAL_CFG_FIRST = 0,
    LTE_SIGNAL_CFG_INTERVAL = 1, ///< Poll interval, microseconds (DTYPE_UINT32)
    LTE_SIGNAL_CFG_LAST = 31
  };

  explicit CIOLteSignal(CDescObject &desc)
    : CIOCommon(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "lte_signal";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;
  int getDataImpl(IODataCmn &data, size_t len) override;

  size_t getDataSize() const override;
  size_t getDataDim() const override;

#ifdef CONFIG_DAWN_IO_NOTIFY
  int getFd() const override;
#endif

  bool isRead() const override
  {
    return true;
  };

  bool isWrite() const override
  {
    return false;
  };

  bool isNotify() const override
  {
#ifdef CONFIG_DAWN_IO_NOTIFY
    return true;
#else
    return false;
#endif
  };

  bool isBatch() const override
  {
    return false;
  };

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_LTE_SIGNAL, SObjectId::DTYPE_INT16>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdInterval(bool rw = false)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_LTE_SIGNAL,
                                 SObjectId::DTYPE_UINT32,
                                 rw,
                                 1,
                                 LTE_SIGNAL_CFG_INTERVAL);
  }
};
} // Namespace dawn
