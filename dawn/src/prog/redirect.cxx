// dawn/src/prog/redirect.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/redirect.hxx"

#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

int CProgRedirect::ioNotifierCb(void *priv, io_ddata_t *data)
{
  SRedirectBind *bind = static_cast<SRedirectBind *>(priv);
  int ret;

  if (bind == nullptr || !bind->active)
    {
      return OK;
    }

  ret = bind->dst->setData(*data);
  if (ret != OK)
    {
      DAWNERR("redirect: dst 0x%" PRIx32 " setData failed: %d\n", bind->dst->getIdV(), ret);
      return ret;
    }

  return OK;
}

int CProgRedirect::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  while (offset < desc.getSize())
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_REDIRECT)
        {
          DAWNERR("redirect: unsupported cfg 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_REDIRECT_CFG_IOBIND:
            {
              const size_t wpe = sizeof(SProgRedirectIOBind) / 4;
              const SProgRedirectIOBind *tmp;
              size_t nbinds;
              size_t i;
              int ret;

              if (item->cfgid.s.size == 0 || item->cfgid.s.size % wpe != 0)
                {
                  DAWNERR("redirect: invalid IOBIND size %d\n", item->cfgid.s.size);
                  return -EINVAL;
                }

              nbinds = item->cfgid.s.size / wpe;
              tmp = reinterpret_cast<const SProgRedirectIOBind *>(item->data);

              for (i = 0; i < nbinds; i++)
                {
                  ret = allocObject(&tmp[i]);
                  if (ret != OK)
                    {
                      return ret;
                    }
                }

              break;
            }

          default:
            {
              DAWNERR("redirect: unsupported cfg 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

int CProgRedirect::allocObject(const SProgRedirectIOBind *cfg)
{
  SRedirectBind *b;

  DAWNINFO("allocate redirect 0x%" PRIx32 " -> 0x%" PRIx32 "\n", cfg->src.v, cfg->dst.v);

  b = new (std::nothrow) SRedirectBind();
  if (!b)
    {
      return -ENOMEM;
    }

  b->cfg = *cfg;
  b->src = nullptr;
  b->dst = nullptr;
  b->active = false;

  setObjectMapItem(b->cfg.src.v, nullptr);
  setObjectMapItem(b->cfg.dst.v, nullptr);
  binds.push_back(b);

  return OK;
}

CProgRedirect::~CProgRedirect()
{
  deinit();
}

int CProgRedirect::configure()
{
  int ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("redirect configure failed: %d\n", ret);
      return ret;
    }

  if (binds.empty())
    {
      DAWNERR("redirect: no bindings configured\n");
      return -EINVAL;
    }

  return OK;
}

int CProgRedirect::init()
{
  for (auto bind : binds)
    {
      int ret;

      bind->src = getIO(bind->cfg.src.v);
      if (!bind->src)
        {
          DAWNERR("redirect: src 0x%" PRIx32 " not found\n", bind->cfg.src.v);
          return -EIO;
        }

      bind->dst = getIO(bind->cfg.dst.v);
      if (!bind->dst)
        {
          DAWNERR("redirect: dst 0x%" PRIx32 " not found\n", bind->cfg.dst.v);
          return -EIO;
        }

      if (!bind->src->isNotify())
        {
          DAWNERR("redirect: src 0x%" PRIx32 " has no notify support\n", bind->src->getIdV());
          return -EINVAL;
        }

      if (bind->src->getDtype() != bind->dst->getDtype() ||
          bind->src->getDataDim() != bind->dst->getDataDim() ||
          bind->src->isTimestamp() != bind->dst->isTimestamp() ||
          bind->src->getDataSize() != bind->dst->getDataSize())
        {
          DAWNERR("redirect: incompatible src/dst IO types 0x%" PRIx32 " -> 0x%" PRIx32 "\n",
                  bind->src->getIdV(),
                  bind->dst->getIdV());
          return -EINVAL;
        }

      ret = prepareWritableTarget(bind->dst, bind->src->getDataDim(), true);
      if (ret != OK)
        {
          DAWNERR(
            "redirect: target prepare failed for 0x%" PRIx32 ": %d\n", bind->dst->getIdV(), ret);
          return ret;
        }
    }

  return OK;
}

int CProgRedirect::deinit()
{
  doStop();

  for (auto bind : binds)
    {
      bind->src = nullptr;
      bind->dst = nullptr;
      delete bind;
    }

  binds.clear();
  return OK;
}

int CProgRedirect::doStart()
{
  for (auto bind : binds)
    {
      int ret;

      bind->active = true;
      ret = bind->src->setNotifier(ioNotifierCb, 0, bind);
      if (ret != OK)
        {
          DAWNERR("redirect: setNotifier failed for 0x%" PRIx32 ": %d\n", bind->src->getIdV(), ret);
          return ret;
        }
    }

  return OK;
}

int CProgRedirect::doStop()
{
  for (auto bind : binds)
    {
      if (bind->src != nullptr)
        {
          bind->src->setNotifier(nullptr, 0, nullptr);
        }

      bind->active = false;
    }

  return OK;
}

bool CProgRedirect::hasThread() const
{
  return false;
}
