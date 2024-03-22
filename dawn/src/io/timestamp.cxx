// dawn/src/io/timestamp.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/timestamp.hxx"

using namespace dawn;

int CIOTimestamp::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;
  size_t i = 0;

  for (i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls == CIOCommon::IO_CLASS_ANY)
        {
          offset += cfgCmnOffset(item);
          continue;
        }

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_TIMESTAMP)
        {
          DAWNERR("unsupported timestamp cfg 0x08%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case CIOTimestamp::IO_TIMESTAMP_CFG_INTERVAL:
            {
              const uint32_t *tmp = &item->data[0];

              timfd_interval(*tmp);
              offset += 1 + item->cfgid.s.size;
              break;
            }

          default:
            {
              DAWNERR("unsupported timestamp objectCfg 0x%" PRIx32 "\n", item->cfgid.v);
              DAWNERR("Unsupported timestamp source type\n");
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIOTimestamp::~CIOTimestamp()
{
  deinit();
}

int CIOTimestamp::configure()
{
  int ret;

  // Configure

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      return ret;
    }

  return timfd_init();
}

int CIOTimestamp::deinit()
{
  // Stop IO

  stop();

  return OK;
}

int CIOTimestamp::getDataImpl(IODataCmn &data, size_t len)
{
  for (size_t i = 0; i < len; i++)
    {
      uint64_t ts = getTimestamp();
      std::memcpy(data.getDataPtr(i), &ts, tlen);

#ifdef CONFIG_DAWN_IO_TIMESTAMP
      // This is here only to be consistent across IO implementations

      if (isTimestamp())
        {
          data.getTs() = ts;
        }
#endif
    }

  // Clear notification event from fd

  timfd_ack();

  return OK;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CIOTimestamp::getFd() const
{
  return timfd_fd();
};
#endif

int CIOTimestamp::doStart()
{
  return timfd_start();
}

int CIOTimestamp::doStop()
{
  return timfd_stop();
}

size_t CIOTimestamp::getDataSize() const
{
  // Data size

  return tlen;
}

size_t CIOTimestamp::getDataDim() const
{
  // Data dimmention

  return 1;
}
