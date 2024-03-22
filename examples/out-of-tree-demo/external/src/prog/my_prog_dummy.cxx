// examples/out-of-tree-demo/external/src/prog/my_prog_dummy.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <inttypes.h>

#include "my_prog_dummy.hxx"

#include "dawn/debug.hxx"

using namespace oot_demo;

int CProgMyDummy::configure(void)
{
  const dawn::SObjectCfg::SObjectCfgItem *item = nullptr;
  const dawn::SObjectId::UObjectId *ids;
  size_t offset = 0;
  size_t i;

  for (i = 0; i < getDesc().getSize(); i++)
    {
      item = getDesc().objectCfgItemNext(offset);

      if (item->cfgid.s.cls != PROG_CLASS_MY_DUMMY)
        {
          DAWNERR("CProgMyDummy: unsupported cfg 0x%08" PRIx32 "\n",
                  item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_MY_DUMMY_CFG_IOBIND:
            {
              size_t j;

              ids = reinterpret_cast<const dawn::SObjectId::UObjectId *>(
                  item->data);
              for (j = 0; j < item->cfgid.s.size; j++)
                {
                  setObjectMapItem(ids[j].v, nullptr);
                }
              break;
            }

          case PROG_MY_DUMMY_CFG_TAG:
            tag = item->data[0];
            DAWNINFO("CProgMyDummy: configured tag 0x%08" PRIx32 "\n",
                     tag);
            break;

          default:
            DAWNERR("CProgMyDummy: unsupported cfg 0x%08" PRIx32 "\n",
                    item->cfgid.v);
            return -EINVAL;
        }
    }

  return OK;
}
