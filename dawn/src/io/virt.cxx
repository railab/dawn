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
  // Not initialized yet

  if (!iodata)
    {
      return -EACCES;
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

#ifdef CONFIG_DAWN_IO_TIMESTAMP
      data.getTs(i) = ts;
#endif

      // We use batch=1 storage for iodata now

      std::memcpy(data.getDataPtr(i), iodata->getDataPtr(0), dlen * tlen);

      mutex.unlock();
    }

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

  // Set IO data

  ret = setVal(data.getDataPtr(), dlen * tlen);

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
  // Data size

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
  data = new (std::nothrow) io_ddata_t(tlen, dim, batch, getDtype());
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

  DAWNASSERT(dlen * tlen == d, "invalid input");

  mutex.lock();

  std::memcpy(v, iodata->getDataPtr(), d);

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

  DAWNASSERT(dlen * tlen == d, "invalid input");

  mutex.lock();

  std::memcpy(iodata->getDataPtr(), v, d);

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      ts = getTimestamp();
    }
#endif

  mutex.unlock();

#ifdef CONFIG_DAWN_IO_NOTIFY
  // Set notification

  sendNotify(iodata);
#endif

  return OK;
}
