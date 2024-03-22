// dawn/src/proto/nxscope/dummy.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nxscope/dummy.hxx"

#include "dawn/io/common.hxx"
#include "dawn/io/sdata.hxx"

using namespace dawn;

int CProtoNxscopeDummy::configureDesc(const CDescObject &desc)
{
  SObjectCfg::SObjectCfgItem *item;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProtoCommon::PROTO_CLASS_NXSCOPE_DUMMY)
        {
          DAWNERR("unsupported nxscope cfg 0x08%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_NXSCOPE_DUMMY_CFG_IOBIND:
            {
              const size_t wpe = sizeof(SProtoNxscopeIOBind) / 4;
              size_t j;

              for (j = 0; j < item->cfgid.s.size; j++)
                {
                  SProtoNxscopeIOBind *tmp =
                    reinterpret_cast<SProtoNxscopeIOBind *>(item->data + j * wpe);

                  allocObject(tmp);
                }

              break;
            }

          case PROTO_NXSCOPE_DUMMY_CFG_IOBIND2:
            {
              const size_t wpe = sizeof(SProtoNxscopeIOBind2) / 4;
              const size_t entry_count = item->cfgid.s.size / wpe;
              size_t j;

              if (item->cfgid.s.size % wpe != 0)
                {
                  DAWNERR("unsupported nxscope cfg 0x08%" PRIx32 "\n", item->cfgid.v);
                  return -EINVAL;
                }

              for (j = 0; j < entry_count; j++)
                {
                  SProtoNxscopeIOBind2 *tmp =
                    reinterpret_cast<SProtoNxscopeIOBind2 *>(item->data + j * wpe);

                  allocObject(&tmp->bind);
                  allocNames(j, &tmp->name);
                }

              break;
            }

          default:
            {
              DAWNERR("unsupported nxscope cfg 0x08%" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

int CProtoNxscopeDummy::confgureNxscopePriv()
{
  int ret;

  nxsDummyCfg.res = 0;

  DAWNINFO("nxscope dummy\n");

  ret = nxscope_dummy_init(&nxsIntf, &nxsDummyCfg);
  if (ret != OK)
    {
      DAWNERR("nxscope_dummy_init failed: %d\n", ret);
      return ret;
    }

  return ret;
}

CProtoNxscopeDummy::~CProtoNxscopeDummy()
{
}

int CProtoNxscopeDummy::initPriv()
{
  int ret;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      return ret;
    }

  ret = confgureNxscopePriv();
  if (ret != OK)
    {
      return ret;
    }

  ret = configureNxscope();
  if (ret != OK)
    {
      return ret;
    }

  return OK;
}

int CProtoNxscopeDummy::deinitPriv()
{
  nxscope_dummy_deinit(&nxsIntf);
  return OK;
}

int CProtoNxscopeDummy::startPriv()
{
  return OK;
}

int CProtoNxscopeDummy::stopPriv()
{
  return OK;
}
