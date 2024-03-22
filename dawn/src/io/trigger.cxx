// dawn/src/io/trigger.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/trigger.hxx"

#include <errno.h>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"

using namespace dawn;

int CIOTrigger::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;
  size_t i = 0;

  for (i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_TRIGGER)
        {
          DAWNERR("unsupported trigger cfg 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case IO_TRIGGER_CFG_ALLOCOBJ:
            {
              offset += 1;

              for (size_t j = 0; j < item->cfgid.s.size; j++)
                {
                  const SObjectCfg::SObjectCfgItem *obj = desc.objectCfgItemAtOffset(offset);

                  ids.push_back(static_cast<SObjectId::ObjectId>(obj->cfgid.v));

                  offset += 1;
                }

              break;
            }

          case IO_TRIGGER_CFG_ALLOWED:
            {
              const uint32_t *tmp = reinterpret_cast<const uint32_t *>(&item->data);

              allowed = *tmp;
              offset += 2;
              break;
            }

          default:
            {
              DAWNERR("unsupported trigger cfg 0x%" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIOTrigger::~CIOTrigger()
{
  deinit();
}

int CIOTrigger::configure()
{
  int ret;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      return ret;
    }

  return OK;
}

int CIOTrigger::deinit()
{
  return OK;
}

int CIOTrigger::getDataImpl(IODataCmn &data, size_t len)
{
  UNUSED(data);
  UNUSED(len);
  return -ENOTSUP;
}

int CIOTrigger::setDataImpl(IODataCmn &data)
{
  uint8_t cmd = *reinterpret_cast<const uint8_t *>(data.getDataPtr());
  uint32_t mask = 0;
  int ret = OK;

  switch (cmd)
    {
      case CObject::CMD_RESET:
        {
          mask = TRIG_ALLOW_RESET;
          break;
        }

      case CObject::CMD_TRIGGER1:
        {
          mask = TRIG_ALLOW_TRIGGER1;
          break;
        }

      case CObject::CMD_TRIGGER2:
        {
          mask = TRIG_ALLOW_TRIGGER2;
          break;
        }

      case CObject::CMD_TRIGGER3:
        {
          mask = TRIG_ALLOW_TRIGGER3;
          break;
        }

      default:
        {
          DAWNERR("invalid trigger command %d\n", cmd);
          return -EINVAL;
        }
    }

  if (!(allowed & mask))
    {
      DAWNERR("trigger command %d not allowed\n", cmd);
      return -EACCES;
    }

  if (targets.empty())
    {
      DAWNERR("no bound targets\n");
      return -ENOENT;
    }

  for (CObject *target : targets)
    {
      int r = target->trigger(cmd);

      if (r < 0 && r != -ENOTSUP)
        {
          DAWNERR("trigger failed %d\n", r);
          ret = r;
        }
    }

  return ret;
}

size_t CIOTrigger::getDataSize() const
{
  return sizeof(uint8_t);
}

size_t CIOTrigger::getDataDim() const
{
  return 1;
}

int CIOTrigger::bind(CObject *obj)
{
  targets.push_back(obj);
  return OK;
}
