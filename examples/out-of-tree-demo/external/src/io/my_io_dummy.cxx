// examples/out-of-tree-demo/external/src/io/my_io_dummy.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <cstring>
#include <inttypes.h>

#include "my_io_dummy.hxx"

#include "dawn/debug.hxx"

using namespace oot_demo;

int CIOMyDummy::configure(void)
{
  const dawn::SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;
  size_t i;

  for (i = 0; i < getDesc().getSize(); i++)
    {
      item = getDesc().objectCfgItemNext(offset);

      if (item->cfgid.s.cls == dawn::CIOCommon::IO_CLASS_ANY)
        {
          continue;
        }

      if (item->cfgid.s.cls != IO_CLASS_MY_DUMMY)
        {
          DAWNERR("CIOMyDummy: unsupported cfg 0x%08" PRIx32 "\n",
                  item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case IO_MY_DUMMY_CFG_INITVAL:
            value = item->data[0];
            DAWNINFO("CIOMyDummy: configured init value 0x%08" PRIx32 "\n",
                     value);
            break;

          default:
            DAWNERR("CIOMyDummy: unsupported cfg 0x%08" PRIx32 "\n",
                    item->cfgid.v);
            return -EINVAL;
        }
    }

  return OK;
}

int CIOMyDummy::getDataImpl(dawn::IODataCmn &data, size_t len)
{
  size_t i;

  for (i = 0; i < len; i++)
    {
      std::memcpy(data.getDataPtr(i), &value, sizeof(value));
    }

  return OK;
}

int CIOMyDummy::setDataImpl(dawn::IODataCmn &data)
{
  std::memcpy(&value, data.getDataPtr(), sizeof(value));
  DAWNINFO("CIOMyDummy: value updated to 0x%08x\n", value);
  return OK;
}
