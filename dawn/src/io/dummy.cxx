// dawn/src/io/dummy.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <cstring>
#include <errno.h>
#include <inttypes.h>
#include <new>

#include "dawn/io/dummy.hxx"

using namespace dawn;

static size_t dummyInitvalWordsPerValue(size_t tlen)
{
  if (tlen == sizeof(uint64_t))
    {
      return 2;
    }

  return 1;
}

int CIODummy::configureDesc(const CDescObject &desc)
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

      if (item->cfgid.s.cls == CIOCommon::IO_CLASS_ANY)
        {
          offset += cfgCmnOffset(item);
          continue;
        }

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_DUMMY)
        {
          DAWNERR("unsupported dummy cfg 0x08%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case IO_DUMMY_CFG_INITVAL:
            {
              size_t word_count = item->cfgid.s.size;
              size_t word_step = dummyInitvalWordsPerValue(getDtypeSize());
              size_t init_len;

              if (word_count == 0)
                {
                  DAWNERR("Dummy IO init value dimension must be > 0\n");
                  return -EINVAL;
                }

              if (word_count % word_step != 0)
                {
                  DAWNERR("Dummy IO init value word count %zu is invalid "
                          "for element size %zu\n",
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

          case IO_DUMMY_CFG_DIM:
            {
              uint32_t dim = item->data[0];

              if (dim == 0)
                {
                  DAWNERR("Dummy IO dimension must be > 0\n");
                  return -EINVAL;
                }

              parsed_dim = dim;
              parsed_dim_set = true;
              offset += 1 + item->cfgid.s.size;
              break;
            }

          default:
            {
              DAWNERR("unsupported dummy objectCfg 0x%" PRIx32 "\n", item->cfgid.v);
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
      DAWNERR("Dummy IO init value dimension %zu is smaller than data "
              "dimension %zu\n",
              parsed_init_len,
              parsed_dim);
      return -EINVAL;
    }

  dlen = parsed_dim;
  cfglen = parsed_init_len;

  return OK;
}

int CIODummy::setVal(const void *v, size_t d)
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

int CIODummy::applyInitval()
{
  size_t i = 0;
  size_t word_step;

  if (cfgval == nullptr)
    {
      return OK;
    }

  DAWNASSERT(val != nullptr, "buffer not initialized");

  word_step = dummyInitvalWordsPerValue(tlen);

  for (i = 0; i < dlen; i++)
    {
      std::memcpy(static_cast<uint8_t *>(val) + (i * tlen), &cfgval[i * word_step], tlen);
    }

  return OK;
}

CIODummy::~CIODummy()
{
  deinit();
}

int CIODummy::configure()
{
  dlen = 1;
  cfglen = 1;
  cfgval = nullptr;

  return configureDesc(getDesc());
}

int CIODummy::init()
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

  return OK;
}

int CIODummy::deinit()
{
  if (val)
    {
      delete[] static_cast<uint8_t *>(val);
      val = nullptr;
    }

  return OK;
}

int CIODummy::getDataImpl(IODataCmn &data, size_t len)
{
  size_t i = 0;

  for (i = 0; i < len; i++)
    {
      mutex.lock();

#ifdef CONFIG_DAWN_IO_TIMESTAMP
      data.getTs(i) = ts;
#endif
      std::memcpy(data.getDataPtr(i), val, dlen * tlen);

      mutex.unlock();
    }

  return OK;
}

int CIODummy::setDataImpl(IODataCmn &data)
{
  return setVal(data.getDataPtr(), dlen * tlen);
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CIODummy::getFd() const
{
  return -1;
};
#endif

size_t CIODummy::getDataSize() const
{
  return dlen * tlen;
}

size_t CIODummy::getDataDim() const
{
  return dlen;
}
