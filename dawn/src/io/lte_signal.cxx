// dawn/src/io/lte_signal.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/lte_signal.hxx"

#include <cerrno>
#include <cinttypes>
#include <cstring>

#include "dawn/porting/lte.hxx"

using namespace dawn;

int CIOLteSignal::configure()
{
  CDescObject &desc = getDesc();
  const SObjectCfg::SObjectCfgItem *item;
  size_t offset = 0;

#ifdef CONFIG_DAWN_IO_NOTIFY
  interval = 0;
#endif

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls == CIOCommon::IO_CLASS_LTE_SIGNAL)
        {
          if (item->cfgid.s.id == LTE_SIGNAL_CFG_INTERVAL)
            {
              timfd_interval(item->data[0]);
            }
        }
      else if (item->cfgid.s.cls != CIOCommon::IO_CLASS_ANY)
        {
          DAWNERR("unsupported lte_signal cfg 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      offset += cfgCmnOffset(item);
    }

  return OK;
}

int CIOLteSignal::init()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  if (interval == 0)
    {
      DAWNERR("lte_signal requires interval > 0\n");
      return -EINVAL;
    }
#endif

  return timfd_init();
}

int CIOLteSignal::deinit()
{
  timfd_stop();
  return OK;
}

int CIOLteSignal::doStart()
{
  return timfd_start();
}

int CIOLteSignal::doStop()
{
  return timfd_stop();
}

int CIOLteSignal::getDataImpl(IODataCmn &data, size_t len)
{
  int16_t vec[DAWN_LTE_SIGNAL_DIM];
  struct SLteQuality q;
  struct SLteCellinfo cell;
  size_t i;
  int ret;

  // RSRP/RSRQ/SINR from the quality read - the primary metrics. No camped
  // cell / RF off: report no data so a 'latest' wrapper keeps the last value.

  ret = lte_port_get_quality(&q);
  if (ret < 0)
    {
      timfd_ack();
      return ret;
    }

  if (!q.valid)
    {
      timfd_ack();
      return -ENODATA;
    }

  vec[DAWN_LTE_SIGNAL_RSRP] = q.rsrp;
  vec[DAWN_LTE_SIGNAL_RSRQ] = q.rsrq;
  vec[DAWN_LTE_SIGNAL_SINR] = q.sinr;

  // Band from the cell-info read - best-effort (0 when unavailable).

  ret = lte_port_get_cellinfo(&cell);
  vec[DAWN_LTE_SIGNAL_BAND] = (ret == OK && cell.valid) ? static_cast<int16_t>(cell.band) : 0;

  for (i = 0; i < len; i++)
    {
      std::memcpy(data.getDataPtr(i), vec, sizeof(vec));

      if (isTimestamp())
        {
          data.getTs(i) = getTimestamp();
        }
    }

  timfd_ack();
  return OK;
}

size_t CIOLteSignal::getDataSize() const
{
  return DAWN_LTE_SIGNAL_DIM * sizeof(int16_t);
}

size_t CIOLteSignal::getDataDim() const
{
  return DAWN_LTE_SIGNAL_DIM;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CIOLteSignal::getFd() const
{
  return timfd_fd();
}
#endif
