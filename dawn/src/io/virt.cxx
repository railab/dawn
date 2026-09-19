// dawn/src/io/virt.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/virt.hxx"

#include <new>

using namespace dawn;

#ifdef CONFIG_DAWN_IO_NOTIFY
void CIOVirt::sendNotify(io_ddata_t *data)
{
  pfdsLock.lock();

  // Notify all consumers

  for (auto v : vnote)
    {
      if (v.cb)
        {
          v.cb(v.priv, data);
        }
    }

  pfdsLock.unlock();
}
#endif

CIOVirt::~CIOVirt()
{
  // Free data buffer

  if (iodata)
    {
      delete iodata;
    }
}

int CIOVirt::configure()
{
  return OK;
}

int CIOVirt::init()
{
  return OK;
}

int CIOVirt::deinit()
{
  return OK;
}

int CIOVirt::getDataImpl(IODataCmn &data, size_t len)
{
  size_t b;

  // Not initialized yet

  if (!iodata)
    {
      return -EACCES;
    }

  if (blen > 1 && len > blen)
    {
      return -EINVAL;
    }

  for (size_t i = 0; i < len; i++)
    {
      // Notify provider

      if (get_cb)
        {
          get_cb(this, get_cb_priv);
        }

      // Copy data

      mutex.lock();

      // Single-batch storage replicates its value into every slot

      b = blen > 1 ? i : 0;

#ifdef CONFIG_DAWN_IO_TIMESTAMP
      data.getTs(i) = iodata->hasTs ? iodata->getTs(b) : ts;
#endif

      std::memcpy(data.getDataPtr(i), iodata->getDataPtr(b), dlen * tlen);

      mutex.unlock();
    }

  return OK;
}

void CIOVirt::copyBatches(uint8_t *dst,
                          size_t dstStride,
                          const uint8_t *src,
                          size_t srcStride,
                          size_t n)
{
  size_t size = dlen * tlen;

  // Neither side padded (no timestamps) - one flat copy

  if (dstStride == size && srcStride == size)
    {
      std::memcpy(dst, src, size * n);
      return;
    }

  for (size_t b = 0; b < n; b++)
    {
      std::memcpy(dst + b * dstStride, src + b * srcStride, size);
    }
}

size_t CIOVirt::batchStride(IODataCmn &data)
{
  if (data.getBatch() < 2)
    {
      return dlen * tlen;
    }

  return static_cast<uint8_t *>(data.getDataPtr(1)) - static_cast<uint8_t *>(data.getDataPtr(0));
}

int CIOVirt::store(const void *src, size_t stride, IODataCmn *tsSrc)
{
  size_t b;

  mutex.lock();

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      ts = getTimestamp();
    }
#endif

  copyBatches(static_cast<uint8_t *>(iodata->getDataPtr(0)),
              iodata->off,
              static_cast<const uint8_t *>(src),
              stride,
              blen);

  // Source timestamps are kept per batch, otherwise stamp with now

  if (iodata->hasTs)
    {
      for (b = 0; b < blen; b++)
        {
          if (tsSrc != nullptr && tsSrc->hasTimestamp())
            {
              iodata->getTs(b) = tsSrc->getTs(b);
            }
#ifdef CONFIG_DAWN_IO_TIMESTAMP
          else
            {
              iodata->getTs(b) = ts;
            }
#endif
        }
    }

  mutex.unlock();

#ifdef CONFIG_DAWN_IO_NOTIFY
  sendNotify(iodata);
#endif

  return OK;
}

int CIOVirt::setDataImpl(IODataCmn &data)
{
  int ret;

  // Not initialized yet

  if (!iodata)
    {
      return -EACCES;
    }

  // A shorter source would be read past its end; a longer one (batched
  // notifier buffer into a single-slot virt) stores its first blen batches

  if (data.getBatch() < blen)
    {
      DAWNERR("virt batch mismatch: expected %zu got %zu\n", blen, data.getBatch());
      return -EINVAL;
    }

  // Single-slot storage is stamped on write, batches keep the source stamps

  ret = store(data.getDataPtr(0), batchStride(data), blen > 1 ? &data : nullptr);

  // Notify provider

  if (set_cb)
    {
      set_cb(this, set_cb_priv);
    }

  return ret;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CIOVirt::getFd() const
{
  // There is a custom pass-through notification mechanism impelemted for
  // VirtIO, we don't use standard poll notifier here

  return -1;
};
#endif

size_t CIOVirt::getDataSize() const
{
  // Size of a single data item (batches are handled in setVal/getVal)

  return dlen * tlen;
}

size_t CIOVirt::getDataDim() const
{
  // Data dimmention

  return dlen;
}

// VirtIO specific implementation for IIONotifier
int CIOVirt::regNotifier(SIONotifier n)
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  DAWNASSERT(n.io != nullptr, "nullptr pointer");

  pfdsLock.lock();
  if (n.cb == nullptr)
    {
      vnote.clear();
    }
  else
    {
      vnote.push_back(n);
    }

  pfdsLock.unlock();
#endif

  return OK;
}

int CIOVirt::unregNotifier(const SIONotifier &n)
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  DAWNASSERT(n.io != nullptr, "nullptr pointer");

  pfdsLock.lock();

  for (auto it = vnote.begin(); it != vnote.end(); ++it)
    {
      if (it->io == n.io && it->cb == n.cb && it->priv == n.priv)
        {
          vnote.erase(it);
          pfdsLock.unlock();
          return OK;
        }
    }

  pfdsLock.unlock();
  return -ENOENT;
#else
  UNUSED(n);
  return -ENOTSUP;
#endif
}

int CIOVirt::initialize(size_t dim, size_t batch, bool notify)
{
  io_ddata_t *data;

  noteSupport = notify;
  dlen = dim;
  blen = batch;
  data = new (std::nothrow) io_ddata_t(tlen, dim, batch, getDtype(), isTimestamp());
  if (data == nullptr || !data->isAllocated())
    {
      delete data;
      DAWNERR("failed to allocate data\n");
      return -ENOMEM;
    }

  mutex.lock();
  delete iodata;
  iodata = data;
  mutex.unlock();

#ifdef CONFIG_DAWN_IO_NOTIFY
  setNotifyBatch(batch);
  bindNotifier(this);
#endif

  return OK;
}

// Set callback for setData. This is not for IO notifications!

int CIOVirt::setCallbackSet(CIOVirt::virtCB cb, void *priv)
{
  set_cb = cb;
  set_cb_priv = priv;

  return OK;
}

// Set callback for getData. This is not for IO notifications!

int CIOVirt::setCallbackGet(CIOVirt::virtCB cb, void *priv)
{
  get_cb = cb;
  get_cb_priv = priv;

  return OK;
}

int CIOVirt::getVal(void *v, size_t d)
{
  if (!iodata)
    {
      return -EACCES;
    }

  DAWNASSERT(dlen * tlen * blen == d, "invalid input");

  mutex.lock();

  copyBatches(static_cast<uint8_t *>(v),
              dlen * tlen,
              static_cast<const uint8_t *>(iodata->getDataPtr(0)),
              iodata->off,
              blen);

  mutex.unlock();

  return OK;
}

int CIOVirt::setVal(const void *v, size_t d)
{
  // Not initialized yet

  if (!iodata)
    {
      return -EACCES;
    }

  DAWNASSERT(dlen * tlen * blen == d, "invalid input");

  return store(v, dlen * tlen, nullptr);
}
