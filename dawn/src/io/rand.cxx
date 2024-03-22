// dawn/src/io/rand.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/rand.hxx"

#include <fcntl.h>
#include <new>
#include <unistd.h>

using namespace dawn;

#define RAND_DEVPATH "/dev/urandom"

int CIORand::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;
  size_t i = 0;

  for (i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      switch (item->cfgid.s.cls)
        {
          case CIOCommon::IO_CLASS_RAND:
            {
              switch (item->cfgid.s.id)
                {
                  case CIORand::IO_RAND_CFG_INTERVAL:
                    {
                      const uint32_t *tmp = &item->data[0];

                      timfd_interval(*tmp);

                      // Cfgid + data

                      offset += 1 + item->cfgid.s.size;
                      break;
                    }

                  default:
                    {
                      DAWNERR("unsupported rand objectCfg 0x%" PRIx32 "\n", item->cfgid.v);
                      DAWNERR("Unsupported random source type\n");
                      return -EINVAL;
                    }
                }

              break;
            }

          case CIOCommon::IO_CLASS_ANY:
            {
              offset += cfgCmnOffset(item);
              break;
            }

          default:
            {
              DAWNERR("unsupported rand cfg 0x08%" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIORand::~CIORand()
{
  deinit();
}

int CIORand::configure()
{
  int ret;

  val = new (std::nothrow) uint8_t[sizeof(tlen)]();
  if (!val)
    {
      DAWNERR("new failed\n");
      return -ENOMEM;
    }

  // Configure

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      return ret;
    }

  fd = open(RAND_DEVPATH, O_RDONLY);
  if (fd < 0)
    {
      DAWNERR("failed to open %s\n", "/dev/urandom");
      return -errno;
    }

  return timfd_init();
}

int CIORand::deinit()
{
  // Stop IO

  stop();

  // Free data buffer

  if (val)
    {
      free(val);
    }

  return OK;
}

int CIORand::getDataImpl(IODataCmn &data, size_t len)
{
  for (size_t i = 0; i < len; i++)
    {
      ssize_t ret;

      ret = read(fd, data.getDataPtr(i), tlen);
      if (ret < 0)
        {
          DAWNERR("rand read failed %d\n", -errno);
        }

#ifdef CONFIG_DAWN_IO_RAND
      // This is here only to be consistent across IO implementations

      if (isRand())
        {
          data.getTs() = getTimestamp();
        }
#endif
    }

  // Clear notification event from fd

  timfd_ack();

  return OK;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CIORand::getFd() const
{
  return timfd_fd();
};
#endif

int CIORand::doStart()
{
  return timfd_start();
}

int CIORand::doStop()
{
  return timfd_stop();
}

size_t CIORand::getDataSize() const
{
  // Data size

  return tlen;
}

size_t CIORand::getDataDim() const
{
  // Data dimmention

  return 1;
}
