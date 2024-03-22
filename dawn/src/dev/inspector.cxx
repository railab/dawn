// dawn/src/dev/inspector.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/dev/inspector.hxx"

#ifdef CONFIG_DAWN_INSPECT

#  include "dawn/common/object.hxx"
#  include "dawn/io/common.hxx"
#  include "dawn/io/handler.hxx"
#  include "dawn/prog/common.hxx"
#  include "dawn/prog/handler.hxx"
#  include "dawn/proto/common.hxx"
#  include "dawn/proto/handler.hxx"

using namespace dawn;

//***************************************************************************
// Private Data
//***************************************************************************

CDevInspector *CDevInspector::singleton = nullptr;

//***************************************************************************
// Public Functions
//***************************************************************************

void CDevInspector::registerIOHandler(const CIOHandler *handler)
{
  io_handler = handler;
}

void CDevInspector::registerProgHandler(const CProgHandler *handler)
{
  prog_handler = handler;
}

void CDevInspector::registerProtoHandler(const CProtoHandler *handler)
{
  proto_handler = handler;
}

size_t CDevInspector::getIOCount() const
{
  if (io_handler == nullptr)
    {
      return 0;
    }

  return io_handler->getObjects().size();
}

size_t CDevInspector::getProgCount() const
{
  if (prog_handler == nullptr)
    {
      return 0;
    }

  return prog_handler->getObjects().size();
}

size_t CDevInspector::getProtoCount() const
{
  if (proto_handler == nullptr)
    {
      return 0;
    }

  return proto_handler->getObjects().size();
}

const CIOCommon *CDevInspector::getIO(size_t index) const
{
  if (io_handler == nullptr)
    {
      return nullptr;
    }

  const auto &objects = io_handler->getObjects();

  if (index >= objects.size())
    {
      return nullptr;
    }

  return objects[index];
}

const CObject *CDevInspector::getProg(size_t index) const
{
  if (prog_handler == nullptr)
    {
      return nullptr;
    }

  const auto &objects = prog_handler->getObjects();

  if (index >= objects.size())
    {
      return nullptr;
    }

  return objects[index];
}

const CObject *CDevInspector::getProto(size_t index) const
{
  if (proto_handler == nullptr)
    {
      return nullptr;
    }

  const auto &objects = proto_handler->getObjects();

  if (index >= objects.size())
    {
      return nullptr;
    }

  return objects[index];
}

const CObject *CDevInspector::findObject(uint32_t objid) const
{
  const CObject *obj;
  SObjectId::UObjectId uobjid;

  uobjid.v = objid;

  // Check IO handler

  if (io_handler != nullptr)
    {
      obj = io_handler->getObjectById(uobjid);
      if (obj != nullptr)
        {
          return obj;
        }
    }

  // Check PROG handler

  if (prog_handler != nullptr)
    {
      obj = prog_handler->getObjectById(uobjid);
      if (obj != nullptr)
        {
          return obj;
        }
    }

  // Check PROTO handler

  if (proto_handler != nullptr)
    {
      obj = proto_handler->getObjectById(uobjid);
      if (obj != nullptr)
        {
          return obj;
        }
    }

  return nullptr;
}

size_t CDevInspector::getProgIOBindings(size_t prog_index,
                                        const CIOCommon **io_list,
                                        size_t max_size) const
{
  const CObject *prog;
  const CProgCommon *prog_common;
  const std::map<SObjectId::ObjectId, CIOCommon *> *io_map;
  size_t count;

  prog = getProg(prog_index);
  if (prog == nullptr)
    {
      return 0;
    }

  prog_common = static_cast<const CProgCommon *>(prog);
  io_map = &prog_common->getIOMap();

  if (io_map->empty())
    {
      return 0;
    }

  count = 0;
  for (const auto &pair : *io_map)
    {
      if (count >= max_size)
        {
          break;
        }

      io_list[count] = pair.second;
      count++;
    }

  return count;
}

size_t CDevInspector::getProtoBindings(size_t proto_index,
                                       const CObject **obj_list,
                                       size_t max_size) const
{
  const CObject *proto;
  const CProtoCommon *proto_common;
  const std::map<SObjectId::ObjectId, CIOCommon *> *obj_map;
  size_t count;

  proto = getProto(proto_index);
  if (proto == nullptr)
    {
      return 0;
    }

  proto_common = static_cast<const CProtoCommon *>(proto);
  obj_map = &proto_common->getIOMap();

  if (obj_map->empty())
    {
      return 0;
    }

  count = 0;
  for (const auto &pair : *obj_map)
    {
      if (count >= max_size)
        {
          break;
        }

      obj_list[count] = pair.second;
      count++;
    }

  return count;
}

#endif // CONFIG_DAWN_INSPECT
