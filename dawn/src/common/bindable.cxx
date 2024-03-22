// dawn/src/common/bindable.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/bindable.hxx"

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"

using namespace dawn;

void CBindableObject::setIOMapItem(SObjectId::ObjectId id, CIOCommon *io)
{
  ioMap.insert_or_assign(id, io);
}

const std::map<SObjectId::ObjectId, CIOCommon *> &CBindableObject::getIOMap() const
{
  return ioMap;
}

void CBindableObject::setObjectMapItem(SObjectId::ObjectId id, CObject *obj)
{
  switch (SObjectId::objectIdGetType(id))
    {
      case SObjectId::OBJTYPE_IO:
        {
          setIOMapItem(id, static_cast<CIOCommon *>(obj));
          break;
        }

      default:
        {
          DAWNERR("Unsupported object type 0x%x for binding\n", SObjectId::objectIdGetType(id));
          break;
        }
    }
}

CIOCommon *CBindableObject::getIO(SObjectId::ObjectId id)
{
  auto it = ioMap.find(id);
  if (it == ioMap.end())
    {
      return nullptr;
    }

  return it->second;
}

CObject *CBindableObject::getObject(SObjectId::ObjectId id)
{
  if (SObjectId::objectIdGetType(id) == SObjectId::OBJTYPE_IO)
    {
      return getIO(id);
    }

  DAWNERR("unknown object type = 0x%" PRIx32 "\n", id);
  return nullptr;
}
