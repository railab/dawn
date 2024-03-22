// dawn/src/prog/gateway.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/gateway.hxx"

#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"
#include "dawn/io/virt.hxx"

using namespace dawn;

void CProgGateway::io1SetCb(CIOVirt *io, void *priv)
{
  SGatewayBind *b = static_cast<SGatewayBind *>(priv);
  int ret;

  ret = io->getVal(b->iodata->getDataPtr(), b->iodata->getDataSize());
  if (ret != OK)
    {
      DAWNERR("gateway io1SetCb getVal: %d\n", ret);
      return;
    }

  b->vio2->setVal(b->iodata->getDataPtr(), b->iodata->getDataSize());
}

void CProgGateway::io2SetCb(CIOVirt *io, void *priv)
{
  SGatewayBind *b = static_cast<SGatewayBind *>(priv);
  int ret;

  ret = io->getVal(b->iodata->getDataPtr(), b->iodata->getDataSize());
  if (ret != OK)
    {
      DAWNERR("gateway io2SetCb getVal: %d\n", ret);
      return;
    }

  b->vio1->setVal(b->iodata->getDataPtr(), b->iodata->getDataSize());
}

void CProgGateway::io1GetCb(CIOVirt *io, void *priv)
{
  SGatewayBind *b = static_cast<SGatewayBind *>(priv);
  int ret;

  ret = b->vio2->getVal(b->iodata->getDataPtr(), b->iodata->getDataSize());
  if (ret != OK)
    {
      DAWNERR("gateway io1GetCb getVal from io2: %d\n", ret);
      return;
    }

  io->setVal(b->iodata->getDataPtr(), b->iodata->getDataSize());
}

void CProgGateway::io2GetCb(CIOVirt *io, void *priv)
{
  SGatewayBind *b = static_cast<SGatewayBind *>(priv);
  int ret;

  ret = b->vio1->getVal(b->iodata->getDataPtr(), b->iodata->getDataSize());
  if (ret != OK)
    {
      DAWNERR("gateway io2GetCb getVal from io1: %d\n", ret);
      return;
    }

  io->setVal(b->iodata->getDataPtr(), b->iodata->getDataSize());
}

int CProgGateway::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  const SProgGatewayIOBind *tmp;
  size_t offset = 0;
  size_t nbinds;
  size_t j;
  int ret;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_GATEWAY)
        {
          DAWNERR("unsupported gateway cfg 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_GATEWAY_CFG_IOBIND:
            {
              if (item->cfgid.s.size == 0 ||
                  item->cfgid.s.size % (sizeof(SProgGatewayIOBind) / 4) != 0)
                {
                  DAWNERR("invalid IOBIND size %d\n", item->cfgid.s.size);
                  return -EINVAL;
                }

              nbinds = item->cfgid.s.size / (sizeof(SProgGatewayIOBind) / 4);

              tmp = reinterpret_cast<const SProgGatewayIOBind *>(item->data);

              for (j = 0; j < nbinds; j++)
                {
                  ret = allocObject(&tmp[j]);
                  if (ret != OK)
                    {
                      return ret;
                    }
                }

              break;
            }

          default:
            {
              DAWNERR("unsupported gateway cfg 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

int CProgGateway::allocObject(const SProgGatewayIOBind *cfg)
{
  SGatewayBind *b;

  if (cfg->dim == 0)
    {
      DAWNERR(
        "gateway bind 0x%" PRIx32 " <-> 0x%" PRIx32 ": dim must be > 0\n", cfg->io1.v, cfg->io2.v);
      return -EINVAL;
    }

  b = new (std::nothrow) SGatewayBind();
  if (!b)
    {
      return -ENOMEM;
    }

  b->cfg = *cfg;
  b->vio1 = nullptr;
  b->vio2 = nullptr;
  b->iodata = nullptr;

  DAWNINFO("gateway bind 0x%" PRIx32 " <-> 0x%" PRIx32 " flags=0x%08" PRIx32 " dim=%" PRIu32 "\n",
           b->cfg.io1.v,
           b->cfg.io2.v,
           b->cfg.flags,
           b->cfg.dim);

  setObjectMapItem(b->cfg.io1.v, nullptr);
  setObjectMapItem(b->cfg.io2.v, nullptr);
  binds.push_back(b);

  return OK;
}

CProgGateway::~CProgGateway()
{
  deinit();
}

int CProgGateway::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("gateway configure failed: %d\n", ret);
      return ret;
    }

  if (binds.empty())
    {
      DAWNERR("gateway: no IO bindings configured\n");
      return -EINVAL;
    }

  return OK;
}

int CProgGateway::deinit()
{
  size_t i;

  for (i = 0; i < binds.size(); i++)
    {
      delete binds[i]->iodata;
      delete binds[i];
    }

  binds.clear();

  return OK;
}

int CProgGateway::init()
{
  SGatewayBind *b;
  size_t i;
  int ret;

  for (i = 0; i < binds.size(); i++)
    {
      b = binds[i];

      b->vio1 = reinterpret_cast<CIOVirt *>(getIO(b->cfg.io1.v));
      if (!b->vio1)
        {
          DAWNERR("gateway: io1[%zu] 0x%" PRIx32 " not found\n", i, b->cfg.io1.v);
          return -EIO;
        }

      b->vio2 = reinterpret_cast<CIOVirt *>(getIO(b->cfg.io2.v));
      if (!b->vio2)
        {
          DAWNERR("gateway: io2[%zu] 0x%" PRIx32 " not found\n", i, b->cfg.io2.v);
          return -EIO;
        }

      ret = prepareWritableTarget(b->vio1, b->cfg.dim, false);
      if (ret != OK)
        {
          DAWNERR("gateway: vio1[%zu] initialize failed: %d\n", i, ret);
          return ret;
        }

      ret = prepareWritableTarget(b->vio2, b->cfg.dim, false);
      if (ret != OK)
        {
          DAWNERR("gateway: vio2[%zu] initialize failed: %d\n", i, ret);
          return ret;
        }

      b->iodata = b->vio1->ddata_alloc(1);
      if (!b->iodata)
        {
          return -ENOMEM;
        }
    }

  return OK;
}

int CProgGateway::doStart()
{
  SGatewayBind *b;
  size_t i;
  int ret;

  for (i = 0; i < binds.size(); i++)
    {
      b = binds[i];

      if (b->cfg.flags & FLAG_IO1_WRITE)
        {
          ret = b->vio1->setCallbackSet(io1SetCb, b);
          if (ret != OK)
            {
              DAWNERR("gateway[%zu]: io1 set cb failed: %d\n", i, ret);
              return ret;
            }
        }

      if (b->cfg.flags & FLAG_IO2_WRITE)
        {
          ret = b->vio2->setCallbackSet(io2SetCb, b);
          if (ret != OK)
            {
              DAWNERR("gateway[%zu]: io2 set cb failed: %d\n", i, ret);
              return ret;
            }
        }

      if (b->cfg.flags & FLAG_IO1_READ)
        {
          ret = b->vio1->setCallbackGet(io1GetCb, b);
          if (ret != OK)
            {
              DAWNERR("gateway[%zu]: io1 get cb failed: %d\n", i, ret);
              return ret;
            }
        }

      if (b->cfg.flags & FLAG_IO2_READ)
        {
          ret = b->vio2->setCallbackGet(io2GetCb, b);
          if (ret != OK)
            {
              DAWNERR("gateway[%zu]: io2 get cb failed: %d\n", i, ret);
              return ret;
            }
        }
    }

  return OK;
}

int CProgGateway::doStop()
{
  SGatewayBind *b;
  size_t i;
  int ret;

  for (i = 0; i < binds.size(); i++)
    {
      b = binds[i];

      if (b->cfg.flags & FLAG_IO1_WRITE)
        {
          ret = b->vio1->setCallbackSet(nullptr, nullptr);
          if (ret != OK)
            {
              DAWNERR("gateway[%zu]: io1 set cb clear failed: %d\n", i, ret);
              return ret;
            }
        }

      if (b->cfg.flags & FLAG_IO2_WRITE)
        {
          ret = b->vio2->setCallbackSet(nullptr, nullptr);
          if (ret != OK)
            {
              DAWNERR("gateway[%zu]: io2 set cb clear failed: %d\n", i, ret);
              return ret;
            }
        }

      if (b->cfg.flags & FLAG_IO1_READ)
        {
          ret = b->vio1->setCallbackGet(nullptr, nullptr);
          if (ret != OK)
            {
              DAWNERR("gateway[%zu]: io1 get cb clear failed: %d\n", i, ret);
              return ret;
            }
        }

      if (b->cfg.flags & FLAG_IO2_READ)
        {
          ret = b->vio2->setCallbackGet(nullptr, nullptr);
          if (ret != OK)
            {
              DAWNERR("gateway[%zu]: io2 get cb clear failed: %d\n", i, ret);
              return ret;
            }
        }
    }

  return OK;
}
