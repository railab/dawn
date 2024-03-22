// dawn/src/proto/nimble/prph.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/prph.hxx"

#include <algorithm>
#include <climits>
#include <new>

#include "dawn/proto/nimble/prph_aios.hxx"
#include "dawn/proto/nimble/prph_bas.hxx"
#include "dawn/proto/nimble/prph_custom.hxx"
#include "dawn/proto/nimble/prph_dis.hxx"
#include "dawn/proto/nimble/prph_ess.hxx"
#include "dawn/proto/nimble/prph_imds.hxx"
#include "dawn/proto/nimble/prph_ots.hxx"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_npl.h"
#include "nimble/nimble_port.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

using namespace dawn;

#ifdef CONFIG_DAWN_PROTO_NIMBLE_TPS
extern "C" void ble_svc_tps_init();
#endif

int CProtoNimblePrph::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProtoCommon::PROTO_CLASS_NIMBLE_PRPH)
        {
          DAWNERR("Unsupported Nimble config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_NIMBLE_CFG_GAPNAME:
            {
              const char *name = reinterpret_cast<const char *>(item->data);

              CProtoNimbleAdv::setGapName(name, item->cfgid.s.size * 4);
              break;
            }

#ifdef CONFIG_DAWN_PROTO_NIMBLE_DIS
          case PROTO_NIMBLE_CFG_IOBIND_DIS:
            {
              IProtoNimblePrphService *tmp =
                new CProtoNimblePrphDis(item, static_cast<IProtoNimblePrphCb *>(this));
              vservices.push_back(tmp);
              break;
            }
#endif

#ifdef CONFIG_DAWN_PROTO_NIMBLE_BAS
          case PROTO_NIMBLE_CFG_IOBIND_BAS:
            {
              IProtoNimblePrphService *tmp =
                new CProtoNimblePrphBas(item, static_cast<IProtoNimblePrphCb *>(this));
              vservices.push_back(tmp);
              break;
            }
#endif

#ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS
          case PROTO_NIMBLE_CFG_IOBIND_AIOS:
            {
              IProtoNimblePrphService *tmp =
                new CProtoNimblePrphAios(item, static_cast<IProtoNimblePrphCb *>(this));
              vservices.push_back(tmp);
              break;
            }
#endif

#ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS
          case PROTO_NIMBLE_CFG_IOBIND_ESS:
            {
              IProtoNimblePrphService *tmp =
                new CProtoNimblePrphEss(item, static_cast<IProtoNimblePrphCb *>(this));
              vservices.push_back(tmp);
              break;
            }
#endif

#ifdef CONFIG_DAWN_PROTO_NIMBLE_IMDS
          case PROTO_NIMBLE_CFG_IOBIND_IMDS:
            {
              IProtoNimblePrphService *tmp =
                new CProtoNimblePrphImds(item, static_cast<IProtoNimblePrphCb *>(this));
              vservices.push_back(tmp);
              break;
            }
#endif

#ifdef CONFIG_DAWN_PROTO_NIMBLE_OTS
          case PROTO_NIMBLE_CFG_IOBIND_OTS:
            {
              IProtoNimblePrphService *tmp =
                new CProtoNimblePrphOts(item, static_cast<IProtoNimblePrphCb *>(this));
              vservices.push_back(tmp);
              break;
            }
#endif

          case PROTO_NIMBLE_CFG_IOBIND_CUSTOM:
            {
              IProtoNimblePrphService *tmp =
                new CProtoNimblePrphCustom(item, static_cast<IProtoNimblePrphCb *>(this));
              vservices.push_back(tmp);
              break;
            }

          default:
            {
              DAWNERR("Unsupported Nimble config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

void CProtoNimblePrph::servicesDefault()
{
  // Service GAP - mandatory

  ble_svc_gap_init();

  // Service GATT - mandatory

  ble_svc_gatt_init();

#ifdef CONFIG_DAWN_PROTO_NIMBLE_TPS
  // Tx Power Service

  ble_svc_tps_init();
#endif
}

int CProtoNimblePrph::servicesCount()
{
  // Count services to handle by us

  size_t count = std::count_if(
    vSvcDefs.begin(), vSvcDefs.end(), [](const ble_gatt_svc_def *svc) { return svc != nullptr; });
  if (count > UCHAR_MAX - 1)
    {
      return -EOVERFLOW;
    }

  // One more service for the end of array

  return static_cast<int>(count + 1);
}

int CProtoNimblePrph::servicesInit()
{
  int ret;

  ret = ble_gatts_count_cfg(svcDefs);
  if (ret != 0)
    {
      DAWNERR("ble_gatts_count_cfg failed: %d\n", ret);
      return -EIO;
    }

  ret = ble_gatts_add_svcs(svcDefs);
  if (ret != 0)
    {
      DAWNERR("ble_gatts_add_svcs failed: %d\n", ret);
      return -EIO;
    }

  return OK;
}

int CProtoNimblePrph::servicesCreate()
{
  uint8_t i;
  int ret;
  int svcCount;

  if (svcDefs != nullptr)
    {
      return OK;
    }

  // Get number of services that we have to manage

  svcCount = servicesCount();
  if (svcCount < 0)
    {
      DAWNERR("servicesCount failed: %d\n", svcCount);
      return -EIO;
    }

  noAllocServices = static_cast<uint8_t>(svcCount);

  // Allocate memory for services data

  svcDefs = new (std::nothrow) ble_gatt_svc_def[noAllocServices]();
  if (svcDefs == nullptr)
    {
      DAWNERR("Failed to allocate service definitions\n");
      return -ENOMEM;
    }

  // Configure services

  i = 0;
  for (const struct ble_gatt_svc_def *svc : vSvcDefs)
    {
      if (svc != nullptr)
        {
          std::memcpy(&svcDefs[i], svc, sizeof(struct ble_gatt_svc_def));
          i++;
        }
    }

  // Initialize services

  ret = servicesInit();
  if (ret != OK)
    {
      DAWNERR("servicesInit failed: %d\n", ret);
      delete[] svcDefs;
      svcDefs = nullptr;
      noAllocServices = 0;
      return ret;
    }

  return OK;
}

void CProtoNimblePrph::bleSyncCb()
{
  ble_addr_t addr;
  int ret;

  // Generate new non-resolvable private address

  ret = ble_hs_id_gen_rnd(1, &addr);
  if (ret != 0)
    {
      DAWNERR("ble_hs_id_gen_rnd failed: %d\n", ret);
      return;
    }

  // Set generated address

  ret = ble_hs_id_set_rnd(addr.val);
  if (ret != 0)
    {
      DAWNERR("ble_hs_id_set_rnd failed: %d\n", ret);
      return;
    }

  ret = ble_hs_util_ensure_addr(0);
  if (ret != 0)
    {
      DAWNERR("ble_hs_util_ensure_addr failed: %d\n", ret);
      return;
    }

  ret = ble_hs_id_infer_auto(0, &CProtoNimbleAdv::ownAddrType);
  if (ret != 0)
    {
      DAWNERR("ble_hs_id_infer_auto failed: %d\n", ret);
      return;
    }

  CProtoNimbleAdv::startAdvertise();
}

CProtoNimblePrph::~CProtoNimblePrph()
{
  deinit();

  // Delete all services

  for (auto s : vservices)
    {
      delete s;
    }

  vservices.clear();
  vSvcDefs.clear();
  vstart.clear();
}

int CProtoNimblePrph::configure()
{
  int ret;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("Nimble configure failed: %d\n", ret);
      return ret;
    }

  // Initialize port

  nimble_port_init();

  // Initialize default services

  servicesDefault();

  // Init all services

  for (auto s : vservices)
    {
      ret = s->init();
      if (ret < 0)
        {
          DAWNERR("failed to init service %p\n", s);
          return ret;
        }
    }

  return OK;
}

int CProtoNimblePrph::deinit()
{
  // Deinit all services

  for (auto s : vservices)
    {
      int ret = s->deinit();
      if (ret < 0)
        {
          DAWNERR("failed to deinit service %p\n", s);
        }
    }

  if (svcDefs != nullptr)
    {
      delete[] svcDefs;
      svcDefs = nullptr;
      noAllocServices = 0;
    }

  return OK;
}

int CProtoNimblePrph::doStart()
{
  // Start all services

  for (auto s : vservices)
    {
      int ret = s->start();
      if (ret < 0)
        {
          DAWNERR("failed to start service %p\n", s);
          return ret;
        }
    }

  return OK;
}

int CProtoNimblePrph::doStop()
{
  // Start all services

  for (auto s : vservices)
    {
      int ret = s->stop();
      if (ret < 0)
        {
          DAWNERR("failed to stop service %p\n", s);
        }
    }

  return OK;
}

bool CProtoNimblePrph::hasThread() const
{
  return hci.isRunning() || host.isRunning();
}

int CProtoNimblePrph::serviceRegister(struct ble_gatt_svc_def *svc)
{
  // Svc can be nullptr if its natively supported by NimBLE.
  // For other services it must be not nullptr and filled.

  if (vSvcDefs.size() >= static_cast<size_t>(INT_MAX))
    {
      return -EOVERFLOW;
    }

  // Allocate service

  vSvcDefs.push_back(svc);
  vstart.push_back(false);

  // Return id for allocated service

  return static_cast<int>(vSvcDefs.size() - 1);
}

int CProtoNimblePrph::startService(int id)
{
  int ret;

  if (id < 0 || (size_t)id >= vstart.size())
    {
      return -EINVAL;
    }

  // Mark service as started

  vstart.at(id) = true;

  // Check if all are enabled

  if (std::any_of(vstart.begin(), vstart.end(), [](bool en) { return !en; }))
    {
      return OK;
    }

  // If all services all started - start nimble

  // Allocated objects are mapped so we can create services now

  ret = servicesCreate();
  if (ret != OK)
    {
      DAWNERR("servicesCreate failed: %d\n", ret);
      return ret;
    }

  // Start threads

  hci.start();

  ble_hs_cfg.sync_cb = CProtoNimblePrph::bleSyncCb;
  ble_svc_gap_device_name_set(CProtoNimbleAdv::gapName);

  host.start();

  return OK;
};

int CProtoNimblePrph::stopService(int id)
{
  if (id < 0 || (size_t)id >= vstart.size())
    {
      return -EINVAL;
    }

  // Mark service as stopped

  vstart[id] = false;

  // Check if all are disabled

  if (std::any_of(vstart.begin(), vstart.end(), [](bool en) { return en; }))
    {
      return OK;
    }

  // Stop nimble if all services are stopped

  hci.stop();
  host.stop();

  return OK;
};
