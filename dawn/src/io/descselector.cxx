// dawn/src/io/descselector.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/descselector.hxx"

#include <errno.h>

using namespace dawn;

int CIODescSelector::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;
  size_t i = 0;

  for (i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      switch (item->cfgid.s.cls)
        {
          case CIOCommon::IO_CLASS_ANY:
            {
              offset += cfgCmnOffset(item);
              break;
            }

          default:
            {
              DAWNERR("unsupported descselector cfg 0x08%" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIODescSelector::~CIODescSelector()
{
  deinit();
}

int CIODescSelector::configure()
{
  return configureDesc(getDesc());
}

int CIODescSelector::deinit()
{
  return OK;
}

int CIODescSelector::getDataImpl(IODataCmn &data, size_t len)
{
  uint32_t *slot;

  if (len != 1)
    {
      return -EINVAL;
    }

  slot = reinterpret_cast<uint32_t *>(data.getDataPtr());
  *slot = static_cast<uint32_t>(CDescSwitch::getActiveSlot());

  return OK;
}

int CIODescSelector::setDataImpl(IODataCmn &data)
{
  CDevDescriptor::SDescriptorReg reg;
  const uint32_t *bin;
  uint32_t slot;
  int ret;

  slot = *reinterpret_cast<const uint32_t *>(data.getDataPtr());

  if (slot >= CONFIG_DAWN_DESC_SLOTS)
    {
      return -EINVAL;
    }

  if (slot == static_cast<uint32_t>(CDescSwitch::getActiveSlot()))
    {
      return OK;
    }

  ret = CDevDescriptor::getInst()->getDescriptor(static_cast<int>(slot), reg);
  if (ret < 0)
    {
      return ret;
    }

  if (reg.ptr == nullptr || reg.len == 0)
    {
      return -ENODATA;
    }

  if ((reg.len % sizeof(uint32_t)) != 0)
    {
      return -EINVAL;
    }

  bin = static_cast<const uint32_t *>(reg.ptr);

  if (!CDescriptor::binCheckValid(bin, reg.len))
    {
      return -EBADMSG;
    }

  ret = CDescriptor::binValid(bin, reg.len);
  if (ret < 0)
    {
      return ret;
    }

  CDescSwitch::requestSwitch(static_cast<uint8_t>(slot));
  return OK;
}
