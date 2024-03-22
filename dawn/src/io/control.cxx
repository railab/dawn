// dawn/src/io/control.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/control.hxx"

#include <errno.h>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"

using namespace dawn;

int CIOControl::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;
  size_t i = 0;

  for (i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_CONTROL)
        {
          DAWNERR("unsupported control cfg 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case IO_CONTROL_CFG_ALLOCOBJ:
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

          case IO_CONTROL_CFG_ALLOWED:
            {
              const uint32_t *tmp = reinterpret_cast<const uint32_t *>(&item->data);

              allowed = *tmp;
              offset += 2;
              break;
            }

          default:
            {
              DAWNERR("unsupported control cfg 0x%" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIOControl::~CIOControl()
{
  deinit();
}

int CIOControl::configure()
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

int CIOControl::deinit()
{
  return OK;
}

int CIOControl::getDataImpl(IODataCmn &data, size_t len)
{
  uint8_t *tmp;

  UNUSED(len);

  if (targets.empty())
    {
      DAWNERR("no bound targets\n");
      return -ENOENT;
    }

  tmp = reinterpret_cast<uint8_t *>(data.getDataPtr());
  *tmp = static_cast<uint8_t>(targets[0]->getState());

  return OK;
}

int CIOControl::setDataImpl(IODataCmn &data)
{
  int ret = OK;
  uint8_t cmd = *reinterpret_cast<const uint8_t *>(data.getDataPtr());

  if (cmd == 0)
    {
      // Stop command

      if (!(allowed & CTRL_ALLOW_STOP))
        {
          DAWNERR("stop command not allowed\n");
          return -EACCES;
        }

      for (CObject *target : targets)
        {
          int r = target->stop();

          if (r < 0)
            {
              DAWNERR("stop failed %d\n", r);
              ret = r;
            }
        }
    }
  else if (cmd == 1)
    {
      // Start command

      if (!(allowed & CTRL_ALLOW_START))
        {
          DAWNERR("start command not allowed\n");
          return -EACCES;
        }

      for (CObject *target : targets)
        {
          int r = target->start();

          if (r < 0)
            {
              DAWNERR("start failed %d\n", r);
              ret = r;
            }
        }
    }
  else
    {
      DAWNERR("invalid control command %d\n", cmd);
      return -EINVAL;
    }

  return ret;
}

size_t CIOControl::getDataSize() const
{
  return sizeof(uint8_t);
}

size_t CIOControl::getDataDim() const
{
  return 1;
}

int CIOControl::bind(CObject *obj)
{
  targets.push_back(obj);
  return OK;
}
