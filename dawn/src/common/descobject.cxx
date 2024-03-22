// dawn/src/common/descobject.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/descobject.hxx"

#include "dawn/debug.hxx"

using namespace dawn;

SObjectId::UObjectId &CDescObject::getObjectId() const
{
  return cfgobj.objid;
}

SObjectId::ObjectId CDescObject::getObjectIdV() const
{
  return cfgobj.objid.v;
}

uint16_t CDescObject::getObjectCls() const
{
  return cfgobj.objid.s.cls;
}

uint8_t CDescObject::getObjectType() const
{
  return cfgobj.objid.s.type;
}

uint8_t CDescObject::getObjectDtype() const
{
  return cfgobj.objid.s.dtype;
}

SObjectCfg::SObjectCfgData *CDescObject::getCfg() const
{
  return &cfgobj;
}

SObjectCfg::SObjectCfgItem *CDescObject::objectCfgItemAtOffset(size_t offset) const
{
  // NOTE: offset starts from cfg.items
  return reinterpret_cast<SObjectCfg::SObjectCfgItem *>(
    (reinterpret_cast<uint32_t *>(&cfgobj.items)) + offset);
};

SObjectCfg::SObjectCfgItem *CDescObject::objectCfgItemNext(size_t &offset) const
{
  SObjectCfg::SObjectCfgItem *item = objectCfgItemAtOffset(offset);
  offset += 1 + item->cfgid.s.size;
  return item;
}

SObjectCfg::SObjectCfgItem *CDescObject::objectCfgItemId(SObjectCfg::ObjectCfgId id) const
{
  return SObjectCfg::objectCfgFromCfgId(cfgobj, id);
}

size_t CDescObject::getSize() const
{
  return cfgobj.size;
}

size_t CDescObject::getSizeBytes() const
{
  return size;
}

uint32_t CDescObject::getAtOffset(size_t offset) const
{
  // NOTE: offset starts from the beginning of descriptor
  return (reinterpret_cast<uint32_t *>(&cfgobj))[offset];
}
