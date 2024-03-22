// dawn/src/prog/dummy.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/dummy.hxx"

#include "dawn/debug.hxx"

using namespace dawn;

int CProgDummy::configure()
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  const SObjectId::UObjectId *ids;
  size_t offset = 0;

  while (offset < getDesc().getSize())
    {
      item = getDesc().objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_DUMMY)
        {
          DAWNERR("dummy prog: unsupported cfg 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_DUMMY_CFG_IOBIND:
            {
              size_t i;

              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);

              for (i = 0; i < item->cfgid.s.size; i++)
                {
                  setObjectMapItem(ids[i].v, nullptr);
                }

              break;
            }

          default:
            {
              DAWNERR("dummy prog: unsupported cfg 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}
