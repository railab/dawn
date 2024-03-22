// dawn/src/proto/modbus/tcp.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/modbus/tcp.hxx"

#include "dawn/io/common.hxx"

using namespace dawn;

int CProtoModbusTcp::configureDesc(const CDescObject &desc)
{
  SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProtoCommon::PROTO_CLASS_MODBUS_TCP)
        {
          DAWNERR("Unsupported Modbus TCP config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_MODBUS_TCP_CFG_IOBIND:
            {
              for (size_t j = 0; j < item->cfgid.s.size;)
                {
                  SProtoModbusIOBind *tmp = reinterpret_cast<SProtoModbusIOBind *>(item->data + j);

                  allocObject(tmp);
                  j += sizeof(SProtoModbusIOBind) / 4 + tmp->size;
                }

              break;
            }

          case PROTO_MODBUS_TCP_CFG_PORT:
            {
              port = static_cast<uint16_t>(item->data[0]);
              break;
            }

          default:
            {
              DAWNERR("Unsupported Modbus TCP config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

void CProtoModbusTcp::allocObject(SProtoModbusIOBind *alloc)
{
  for (size_t i = 0; i < alloc->size; i++)
    {
      DAWNINFO("allocate object 0x%" PRIx32 "\n", alloc->objid[i]);

      setObjectMapItem(alloc->objid[i], nullptr);
    }

  valloc_push_back(alloc);
};

void CProtoModbusTcp::thread()
{
  DAWNINFO("start modbus tcp thread\n");

  do
    {
      int ret;

      ret = nxmb_poll(mbhandle);
      if (ret < 0 && ret != -ETIMEDOUT && ret != -EAGAIN && ret != -ECONNRESET)
        {
          DAWNERR("nxmb_poll error: %d\n", ret);
          break;
        }
    }
  while (!workerThread().shouldQuit());

  workerThread().markThreadFinished();
}

int CProtoModbusTcp::modbusInitialize()
{
  struct nxmb_config_s config;
  int ret;

  memset(&config, 0, sizeof(config));
  config.unit_id = saddr;
  config.is_client = false;
  config.mode = NXMB_MODE_TCP;
  config.transport.tcp.port = port;
  config.transport.tcp.host = NULL;
  config.transport.tcp.bindaddr = NULL;

  ret = nxmb_create(&mbhandle, &config);
  if (ret < 0)
    {
      DAWNERR("ERROR: nxmb_create failed: %d\n", ret);
      return ret;
    }

  memset(&callbacks, 0, sizeof(callbacks));
#ifdef CONFIG_DAWN_PROTO_MODBUS_COIL
  callbacks.coil_cb = [](FAR uint8_t *buf,
                         uint16_t addr,
                         uint16_t ncoils,
                         enum nxmb_regmode_e mode,
                         FAR void *priv) -> int
    { return static_cast<CProtoModbusRegs *>(priv)->coilsCb(buf, addr, ncoils, mode, priv); };
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_DISCRETE
  callbacks.discrete_cb =
    [](FAR uint8_t *buf, uint16_t addr, uint16_t ndiscrete, FAR void *priv) -> int
    { return static_cast<CProtoModbusRegs *>(priv)->discreteCb(buf, addr, ndiscrete, priv); };
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_INPUT
  callbacks.input_cb = [](FAR uint8_t *buf, uint16_t addr, uint16_t nregs, FAR void *priv) -> int
    { return static_cast<CProtoModbusRegs *>(priv)->inputCb(buf, addr, nregs, priv); };
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_HOLDING
  callbacks.holding_cb = [](FAR uint8_t *buf,
                            uint16_t addr,
                            uint16_t nregs,
                            enum nxmb_regmode_e mode,
                            FAR void *priv) -> int
    { return static_cast<CProtoModbusRegs *>(priv)->holdingCb(buf, addr, nregs, mode, priv); };
#endif
  callbacks.priv = static_cast<CProtoModbusRegs *>(this);

  ret = nxmb_set_callbacks(mbhandle, &callbacks);
  if (ret < 0)
    {
      DAWNERR("ERROR: nxmb_set_callbacks failed: %d\n", ret);
      nxmb_destroy(mbhandle);
      return ret;
    }

  ret = nxmb_enable(mbhandle);
  if (ret < 0)
    {
      DAWNERR("ERROR: nxmb_enable failed: %d\n", ret);
      nxmb_destroy(mbhandle);
      mbhandle = NULL;
      return ret;
    }

  return OK;
}

CProtoModbusTcp::~CProtoModbusTcp()
{
  deinit();
}

int CProtoModbusTcp::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("Modbus TCP configure failed (error %d)\n", ret);
      return ret;
    }

  return OK;
}

int CProtoModbusTcp::init()
{
  int ret;

  ret = modbusInitialize();
  if (ret < 0)
    {
      return ret;
    }

  ret = createRegs();
  if (ret < 0)
    {
      DAWNERR("failed to create registers %d\n", ret);
      if (mbhandle != NULL)
        {
          nxmb_disable(mbhandle);
          nxmb_destroy(mbhandle);
          mbhandle = NULL;
        }
      return ret;
    }

  return OK;
}

int CProtoModbusTcp::deinit()
{
  destroyRegs();

  if (mbhandle != NULL)
    {
      nxmb_disable(mbhandle);
      nxmb_destroy(mbhandle);
      mbhandle = NULL;
    }

  return OK;
}

int CProtoModbusTcp::doStart()
{
  int ret;

  ret = startWorkerThread([this]() { thread(); });
  if (ret < 0)
    {
      DAWNERR("failed to start thread %d\n", ret);
      return ret;
    }

  return OK;
};

int CProtoModbusTcp::doStop()
{
  stopWorkerThread();

  return OK;
};

bool CProtoModbusTcp::hasThread() const
{
  return workerThreadRunning();
}
