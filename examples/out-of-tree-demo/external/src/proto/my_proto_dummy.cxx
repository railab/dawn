// examples/out-of-tree-demo/external/src/proto/my_proto_dummy.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <inttypes.h>

#include "my_proto_dummy.hxx"

#include "dawn/debug.hxx"

using namespace oot_demo;

int CProtoMyDummy::configure(void)
{
  const dawn::SObjectCfg::SObjectCfgItem *item = nullptr;
  const dawn::SObjectId::UObjectId *ids;
  size_t offset = 0;
  size_t i;

  DAWNINFO("CProtoMyDummy: hello from the out-of-tree demo!\n");

  for (i = 0; i < getDesc().getSize(); i++)
    {
      item = getDesc().objectCfgItemNext(offset);

      if (item->cfgid.s.cls != PROTO_CLASS_MY_DUMMY)
        {
          DAWNERR("CProtoMyDummy: unsupported cfg 0x%08" PRIx32 "\n",
                  item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_MY_DUMMY_CFG_IOBIND:
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

          default:
            DAWNERR("CProtoMyDummy: unsupported cfg 0x%08" PRIx32 "\n",
                    item->cfgid.v);
            return -EINVAL;
        }
    }

  return OK;
}
