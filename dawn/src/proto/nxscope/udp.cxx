// dawn/src/proto/nxscope/udp.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nxscope/udp.hxx"

#include "dawn/io/common.hxx"
#include "dawn/io/sdata.hxx"

using namespace dawn;

int CProtoNxscopeUdp::configureDesc(const CDescObject &desc)
{
  SObjectCfg::SObjectCfgItem *item;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProtoCommon::PROTO_CLASS_NXSCOPE_UDP)
        {
          DAWNERR("unsupported nxscope cfg 0x08%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_NXSCOPE_UDP_CFG_IOBIND:
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

          case PROTO_NXSCOPE_UDP_CFG_IOBIND2:
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

          case PROTO_NXSCOPE_UDP_CFG_PORT:
            {
              port = static_cast<uint16_t>(item->data[0]);
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

int CProtoNxscopeUdp::confgureNxscopePriv()
{
  int ret;

  nxsUdpCfg.port = port;
  nxsUdpCfg.nonblock = NXSCOPE_RECV_NONBLOCK;

  DAWNINFO("nxscope udp port = %u\n", nxsUdpCfg.port);

  ret = nxscope_udp_init(&nxsIntf, &nxsUdpCfg);
  if (ret != OK)
    {
      DAWNERR("nxscope_udp_init failed: %d\n", ret);
      return ret;
    }

  return ret;
}

CProtoNxscopeUdp::~CProtoNxscopeUdp()
{
}

int CProtoNxscopeUdp::initPriv()
{
  int ret;

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

int CProtoNxscopeUdp::deinitPriv()
{
  nxscope_udp_deinit(&nxsIntf);
  return OK;
}

int CProtoNxscopeUdp::startPriv()
{
  return OK;
}

int CProtoNxscopeUdp::stopPriv()
{
  return OK;
}
