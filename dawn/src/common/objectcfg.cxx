// dawn/src/common/objectcfg.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/objectcfg.hxx"

#include "dawn/debug.hxx"

using namespace dawn;

SObjectCfg::SObjectCfgItem *SObjectCfg::objectCfgFromCfgId(const SObjectCfgData &cfg,
                                                           ObjectCfgId id)
{
  size_t offset = 0;

  // Objid + size

  offset += 2;

  for (size_t i = 0; i < cfg.size; i++)
    {
      SObjectCfgItem *item = SObjectCfg::objectAtOffset(cfg, offset);

      if (item->cfgid.v == id)
        {
          return item;
        }

      // Cfgid + data

      offset += 1 + item->cfgid.s.size;
    }

  return nullptr;
}

size_t SObjectCfg::getSizeBytes(const SObjectCfgData &cfg)
{
  size_t offset = 0;

  // Objid + size

  offset += 2;

  for (size_t i = 0; i < cfg.size; i++)
    {
      SObjectCfgItem *item = SObjectCfg::objectAtOffset(cfg, offset);

      // Cfgid + data

      offset += 1 + item->cfgid.s.size;
    }

  return offset * 4;
}
