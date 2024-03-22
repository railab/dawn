// dawn/src/proto/modbus/rtu.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/modbus/rtu.hxx"

#include "dawn/io/common.hxx"

using namespace dawn;

int CProtoModbusRtu::configureDesc(const CDescObject &desc)
{
  SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProtoCommon::PROTO_CLASS_MODBUS_RTU)
        {
          DAWNERR("Unsupported Modbus RTU config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_MODBUS_CFG_IOBIND:
            {
              for (size_t j = 0; j < item->cfgid.s.size;)
                {
                  SProtoModbusIOBind *tmp = reinterpret_cast<SProtoModbusIOBind *>(item->data + j);

                  allocObject(tmp);

                  j += sizeof(SProtoModbusIOBind) / 4 + tmp->size;
                }

              break;
            }

          case CProtoModbusRtu::PROTO_MODBUS_CFG_PATH:
            {
              path = reinterpret_cast<const char *>(&item->data);
              break;
            }

          case CProtoModbusRtu::PROTO_MODBUS_CFG_BAUD:
            {
              baud = static_cast<uint32_t>(item->data[0]);
              break;
            }

          default:
            {
              DAWNERR("Unsupported Modbus RTU config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

void CProtoModbusRtu::allocObject(SProtoModbusIOBind *alloc)
{
  for (size_t i = 0; i < alloc->size; i++)
    {
      DAWNINFO("allocate object 0x%" PRIx32 "\n", alloc->objid[i]);

      // Allocate object in map

      setObjectMapItem(alloc->objid[i], nullptr);
    }

  // Store config for later

  valloc_push_back(alloc);
};

void CProtoModbusRtu::thread()
{
  DAWNINFO("start modbus thread\n");

  do
    {
      int ret;

      /* Poll */

      ret = nxmb_poll(mbhandle);
      if (ret < 0 && ret != -ETIMEDOUT && ret != -EAGAIN)
        {
          DAWNERR("nxmb_poll error: %d\n", ret);
          break;
        }
    }
  while (!workerThread().shouldQuit());

  // Mark thread quit done

  workerThread().markThreadFinished();
}

int CProtoModbusRtu::modbusInitialize()
{
  struct nxmb_config_s config;
  int ret;

  memset(&config, 0, sizeof(config));
  config.unit_id = saddr;
  config.is_client = false;
  config.mode = NXMB_MODE_RTU;

  switch (parity)
    {
      case 0:
      case 'N':
        config.transport.serial.parity = NXMB_PAR_NONE;
        break;
      case 1:
      case 'O':
        config.transport.serial.parity = NXMB_PAR_ODD;
        break;
      case 2:
      case 'E':
        config.transport.serial.parity = NXMB_PAR_EVEN;
        break;
      default:
        DAWNERR("ERROR: Invalid parity setting: %d\n", parity);
        return -EINVAL;
    }

  config.transport.serial.devpath = path;
  config.transport.serial.baudrate = baud;

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

//***************************************************************************
// Public Methods
//***************************************************************************

CProtoModbusRtu::~CProtoModbusRtu()
{
  deinit();
}

int CProtoModbusRtu::configure()
{
  int ret;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("Modbus RTU configure failed (error %d)\n", ret);
      return ret;
    }

  return OK;
}

int CProtoModbusRtu::init()
{
  int ret;

  // NxModbus supports multi-instance, no singleton management needed

  // Initialize modbus stack

  ret = modbusInitialize();
  if (ret < 0)
    {
      return ret;
    }

  // Create modbus registers after IO bindings are resolved

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

int CProtoModbusRtu::deinit()
{
  destroyRegs();

  // Release hardware resources

  if (mbhandle != NULL)
    {
      nxmb_disable(mbhandle);
      nxmb_destroy(mbhandle);
      mbhandle = NULL;
    }

  return OK;
}

int CProtoModbusRtu::doStart()
{
  int ret;

  // Start thread

  ret = startWorkerThread([this]() { thread(); });
  if (ret < 0)
    {
      DAWNERR("failed to start thread %d\n", ret);
      return ret;
    }

  return OK;
};

int CProtoModbusRtu::doStop()
{
  /* Stop thread */

  stopWorkerThread();

  return OK;
};

bool CProtoModbusRtu::hasThread() const
{
  return workerThreadRunning();
}
