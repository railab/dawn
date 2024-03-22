// dawn/include/dawn/io/virt.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <mutex>
#include <vector>

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Virtual I/O type for user-provided data and callbacks.
 *
 * Provides a mechanism for user-defined I/O that can be driven by application
 * callbacks.
 */

class CIOVirt
  : public CIOCommon
  , public IIONotifier
{
public:
  typedef void (*virtCB)(CIOVirt *io, void *priv);

  explicit CIOVirt(CDescObject &desc)
    : CIOCommon(desc)
    , iodata(nullptr)
    , set_cb(nullptr)
    , get_cb(nullptr)
    , set_cb_priv(nullptr)
    , get_cb_priv(nullptr)
    , dlen(0)
    , tlen(getDtypeSize())
#ifdef CONFIG_DAWN_IO_TIMESTAMP
    , ts(0)
#endif
    , noteSupport(false)
  {
  }

  ~CIOVirt() override;

  CIOVirt(const CIOVirt &) = delete;
  CIOVirt &operator=(const CIOVirt &) = delete;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "virt";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;

#ifdef CONFIG_DAWN_IO_NOTIFY
  int getFd() const override;
#endif

  size_t getDataSize() const override;
  size_t getDataDim() const override;

  bool isRead() const override
  {
    return true;
  };

  bool isWrite() const override
  {
    return true;
  };

  bool isNotify() const override
  {
    return this->noteSupport;
  };

  bool isBatch() const override
  {
    return false;
  };

  int regNotifier(SIONotifier n) override;
  int unregNotifier(const SIONotifier &n);

  int initialize(size_t dim, size_t batch = 1, bool note = false);
  int setCallbackSet(virtCB cb, void *priv);
  int setCallbackGet(virtCB cb, void *priv);
  int setVal(const void *v, size_t d);
  int getVal(void *v, size_t d);

  using ObjectIdHelper =
    CIOCommon::IOObjectIdHelper<CIOCommon::IO_CLASS_VIRT, SObjectId::DTYPE_UINT32>;

  constexpr static SObjectId::ObjectId objectId(SObjectId::EObjectDataType dtype,
                                                bool ts,
                                                uint16_t inst)
  {
    return ObjectIdHelper::create(dtype, ts, inst);
  }

private:
  std::mutex mutex;               ///< Mutex protecting data access and callback registration.
  io_ddata_t *iodata;             ///< Internal data storage.
  virtCB set_cb;                  ///< SetData operation callback.
  virtCB get_cb;                  ///< GetData operation callback.
  void *set_cb_priv;              ///< Private data for setData callback.
  void *get_cb_priv;              ///< Private data for getData callback.
  size_t dlen;                    ///< Data size in bytes.
  size_t tlen;                    ///< Type size in bytes.
#ifdef CONFIG_DAWN_IO_TIMESTAMP
  uint64_t ts;                    ///< Timestamp of last data update.
#endif
  bool noteSupport;               ///< Whether notification support is enabled.
#ifdef CONFIG_DAWN_IO_NOTIFY
  std::vector<SIONotifier> vnote; ///< Registered notifiers for data change events.
  std::mutex pfdsLock;            ///< Mutex protecting notifier registration.
#endif

#ifdef CONFIG_DAWN_IO_NOTIFY
  void sendNotify(io_ddata_t *data);
#endif
};
} // Namespace dawn
