// dawn/src/io/dummy_notify.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <cstring>
#include <errno.h>
#include <inttypes.h>
#include <new>
#include <sys/timerfd.h>

#include "dawn/io/dummy_notify.hxx"

using namespace dawn;

static size_t dummyNotifyInitvalWordsPerValue(size_t tlen)
{
  if (tlen == sizeof(uint64_t))
    {
      return 2;
    }

  return 1;
}

int CIODummyNotify::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t parsed_dim = 1;
  bool parsed_dim_set = false;
  size_t parsed_init_len = 1;
  size_t offset = 0;
  size_t i = 0;

  for (i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      switch (item->cfgid.s.cls)
        {
          case CIOCommon::IO_CLASS_ANY:
            {
              offset += cfgCmnOffset(item);
              break;
            }

          case CIOCommon::IO_CLASS_DUMMY_NOTIFY:
            {
              switch (item->cfgid.s.id)
                {
                  case IO_DUMMY_NOTIFY_CFG_INITVAL:
                    {
                      size_t word_count = item->cfgid.s.size;
                      size_t word_step = dummyNotifyInitvalWordsPerValue(getDtypeSize());
                      size_t init_len;

                      if (word_count == 0)
                        {
                          DAWNERR("Dummy notify IO init value "
                                  "dimension must be > 0\n");
                          return -EINVAL;
                        }

                      if (word_count % word_step != 0)
                        {
                          DAWNERR("Dummy notify IO init value word count "
                                  "%zu is invalid for element size %zu\n",
                                  word_count,
                                  getDtypeSize());
                          return -EINVAL;
                        }

                      init_len = word_count / word_step;
                      cfgval = &item->data[0];
                      parsed_init_len = init_len;

                      offset += 1 + word_count;
                      break;
                    }

                  case IO_DUMMY_NOTIFY_CFG_INTERVAL:
                    {
                      const uint32_t *tmp = &item->data[0];

                      timfd_interval(*tmp);
                      offset += 1 + item->cfgid.s.size;
                      break;
                    }

                  case IO_DUMMY_NOTIFY_CFG_DIM:
                    {
                      uint32_t dim = item->data[0];

                      if (dim == 0)
                        {
                          DAWNERR("Dummy notify IO dimension must be > 0\n");
                          return -EINVAL;
                        }

                      parsed_dim = dim;
                      parsed_dim_set = true;
                      offset += 1 + item->cfgid.s.size;
                      break;
                    }

                  case IO_DUMMY_NOTIFY_CFG_NOTIFY_ON_WRITE:
                    {
                      notifyOnWrite = item->data[0] != 0;
                      offset += 1 + item->cfgid.s.size;
                      break;
                    }

                  default:
                    {
                      DAWNERR("unsupported dummy_notify objectCfg "
                              "0x%" PRIx32 "\n",
                              item->cfgid.v);
                      return -EINVAL;
                    }
                }

              break;
            }

          default:
            {
              DAWNERR("unsupported dummy_notify cfg 0x08%" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  if (!parsed_dim_set && cfgval != nullptr)
    {
      parsed_dim = parsed_init_len;
    }

  if (cfgval != nullptr && parsed_init_len < parsed_dim)
    {
      DAWNERR("Dummy notify IO init value dimension %zu is smaller than "
              "data dimension %zu\n",
              parsed_init_len,
              parsed_dim);
      return -EINVAL;
    }

  dlen = parsed_dim;
  cfglen = parsed_init_len;

  return OK;
}

int CIODummyNotify::setVal(const void *v, size_t d)
{
  DAWNASSERT(dlen * tlen == d, "invalid input");
  DAWNASSERT(val != nullptr, "buffer not initialized");

  mutex.lock();

  std::memcpy(val, v, d);

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      ts = getTimestamp();
    }
#endif

  mutex.unlock();

  return OK;
}

int CIODummyNotify::applyInitval()
{
  size_t i = 0;
  size_t word_step;

  if (cfgval == nullptr)
    {
      return OK;
    }

  DAWNASSERT(val != nullptr, "buffer not initialized");

  word_step = dummyNotifyInitvalWordsPerValue(tlen);

  for (i = 0; i < dlen; i++)
    {
      std::memcpy(static_cast<uint8_t *>(val) + (i * tlen), &cfgval[i * word_step], tlen);
    }

  return OK;
}

CIODummyNotify::~CIODummyNotify()
{
  deinit();
}

int CIODummyNotify::configure()
{
  dlen = 1;
  cfglen = 1;
  cfgval = nullptr;
  notifyOnWrite = false;
#ifdef CONFIG_DAWN_IO_NOTIFY
  interval = 0;
#endif

  return configureDesc(getDesc());
}

int CIODummyNotify::init()
{
  int ret;

  val = new (std::nothrow) uint8_t[dlen * tlen]();
  if (!val)
    {
      DAWNERR("new failed\n");
      return -ENOMEM;
    }

  ret = applyInitval();
  if (ret != OK)
    {
      delete[] static_cast<uint8_t *>(val);
      val = nullptr;
      return ret;
    }

  writeData = new (std::nothrow) io_ddata_t(tlen, dlen, 1, getDtype(), isTimestamp());
  if (writeData == nullptr || !writeData->isAllocated())
    {
      delete writeData;
      writeData = nullptr;
      delete[] static_cast<uint8_t *>(val);
      val = nullptr;
      return -ENOMEM;
    }

#ifdef CONFIG_DAWN_IO_NOTIFY
  if (interval == 0)
    {
      DAWNERR("dummy_notify requires interval > 0\n");
      delete writeData;
      writeData = nullptr;
      delete[] static_cast<uint8_t *>(val);
      val = nullptr;
      return -EINVAL;
    }
#endif

  ret = timfd_init();
  if (ret != OK)
    {
      delete writeData;
      writeData = nullptr;
      delete[] static_cast<uint8_t *>(val);
      val = nullptr;
      return ret;
    }

  return OK;
}

int CIODummyNotify::deinit()
{
  stop();

  if (val)
    {
      delete[] static_cast<uint8_t *>(val);
      val = nullptr;
    }

  delete writeData;
  writeData = nullptr;

  return OK;
}

int CIODummyNotify::getDataImpl(IODataCmn &data, size_t len)
{
  size_t i;

  for (i = 0; i < len; i++)
    {
      mutex.lock();

#ifdef CONFIG_DAWN_IO_TIMESTAMP
      data.getTs(i) = ts;
#endif
      std::memcpy(data.getDataPtr(i), val, dlen * tlen);

      mutex.unlock();
    }

  timfd_ack();
  return OK;
}

int CIODummyNotify::setDataImpl(IODataCmn &data)
{
  int ret = setVal(data.getDataPtr(), dlen * tlen);

#ifdef CONFIG_DAWN_IO_NOTIFY
  if (ret == OK && notifyOnWrite && writeData != nullptr)
    {
      mutex.lock();

#  ifdef CONFIG_DAWN_IO_TIMESTAMP
      if (isTimestamp())
        {
          writeData->getTs(0) = ts;
        }
#  endif

      std::memcpy(writeData->getDataPtr(), val, dlen * tlen);

      mutex.unlock();

      (void)notifyData(writeData);
    }
#endif

  return ret;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CIODummyNotify::getFd() const
{
  return timfd_fd();
}

int CIODummyNotify::notify()
{
  struct itimerspec tms;
  int ret;

  if (timfd_fd() < 0)
    {
      return -EINVAL;
    }

  tms.it_value.tv_sec = 0;
  tms.it_value.tv_nsec = 1;
  tms.it_interval.tv_sec = 0;
  tms.it_interval.tv_nsec = 0;

  ret = timerfd_settime(timfd_fd(), 0, &tms, NULL);
  if (ret != OK)
    {
      DAWNERR("timerfd_settime notify failed\n");
    }

  return ret;
}
#endif

int CIODummyNotify::doStart()
{
  return timfd_start();
}

int CIODummyNotify::doStop()
{
  return timfd_stop();
}

size_t CIODummyNotify::getDataSize() const
{
  return dlen * tlen;
}

size_t CIODummyNotify::getDataDim() const
{
  return dlen;
}
