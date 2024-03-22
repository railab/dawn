// dawn/src/proto/nxscope/serial.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nxscope/serial.hxx"

#include "dawn/io/common.hxx"
#include "dawn/io/sdata.hxx"

using namespace dawn;

int CProtoNxscopeSerial::configureDesc(const CDescObject &desc)
{
  SObjectCfg::SObjectCfgItem *item;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProtoCommon::PROTO_CLASS_NXSCOPE_SERIAL)
        {
          DAWNERR("unsupported nxscope cfg 0x08%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_NXSCOPE_SERIAL_CFG_IOBIND:
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

          case PROTO_NXSCOPE_SERIAL_CFG_IOBIND2:
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

          case CProtoNxscopeSerial::PROTO_NXSCOPE_SERIAL_CFG_PATH:
            {
              path = reinterpret_cast<const char *>(&item->data);
              break;
            }

          case CProtoNxscopeSerial::PROTO_NXSCOPE_SERIAL_CFG_BAUD:
            {
              baud = static_cast<uint32_t>(item->data[0]);
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

int CProtoNxscopeSerial::confgureNxscopePriv()
{
  int ret;

  // NOTE: nxscope_ser_cfg_s.path is declared FAR char* (non-const)
  // in the C API, but nxscope_ser_init() only reads it to open the
  // device — it never modifies the string. Cast is safe.
  nxsSerCfg.path = const_cast<char *>(path);
  nxsSerCfg.baud = baud;
  nxsSerCfg.nonblock = NXSCOPE_RECV_NONBLOCK;

  DAWNINFO("nxscope serial path = %s baud = %ld\n", nxsSerCfg.path, nxsSerCfg.baud);

  ret = nxscope_ser_init(&nxsIntf, &nxsSerCfg);
  if (ret != OK)
    {
      DAWNERR("nxscope_ser_init failed: %d\n", ret);
      return ret;
    }
  serInit = true;

  return ret;
}

CProtoNxscopeSerial::~CProtoNxscopeSerial()
{
}

int CProtoNxscopeSerial::initPriv()
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

int CProtoNxscopeSerial::deinitPriv()
{
  if (serInit)
    {
      nxscope_ser_deinit(&nxsIntf);
      serInit = false;
    }

  return OK;
}

int CProtoNxscopeSerial::startPriv()
{
  return OK;
}

int CProtoNxscopeSerial::stopPriv()
{
  if (serInit)
    {
      nxscope_ser_deinit(&nxsIntf);
      serInit = false;
    }

  return OK;
}
