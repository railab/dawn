// dawn/src/proto/handler.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/handler.hxx"

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/handler.hxx"

#ifdef CONFIG_DAWN_INSPECT
#  include "dawn/dev/inspector.hxx"
#endif

using namespace dawn;

void CProtoHandler::objallocPriv(CHandler &obj, CDescObject &desc)
{
  // Allocate object

  if (SObjectId::objectIsProto(desc.getObjectId()))
    {
      CProtoHandler &h = static_cast<CProtoHandler &>(obj);
      CProtoCommon *tmp = nullptr;

      tmp = h.create(desc);
      if (tmp == nullptr)
        {
          DAWNERR("Failed to create PROTO object 0x%" PRIx32 "\n", desc.getObjectIdV());
          h.allocError = -ENOMEM;
          return;
        }

      DAWNINFO("created PROTO: 0x%" PRIx32 " %p\n", desc.getObjectIdV(), tmp);
      if (h.registerObject(tmp) < 0)
        {
          return;
        }
    }
}

CProtoCommon *CProtoHandler::create(CDescObject &desc)
{
  CProtoCommon *ret = nullptr;

  // User factory has priority

  if (userFactory != nullptr)
    {
      ret = userFactory->create(desc);
      if (ret != nullptr)
        {
          return ret;
        }
    }

  return factory.create(desc);
}

CIOCommon *CProtoHandler::getIO(SObjectId::ObjectId id)
{
  SObjectId::UObjectId uid;
  uid.v = id;
  return ioHandler->getIO(uid);
}

int CProtoHandler::objalloc(CDescriptor &desc)
{
  return CGenericHandler<CProtoCommon>::objalloc(desc, objallocPriv);
}

int CProtoHandler::init(CDescriptor &desc, CIOHandler *io, IProtoFactory *f)
{
  int ret;

  // Set user factory
  userFactory = f;

  // Connect handlers
  ioHandler = io;

  // Allocate objects from descriptor
  ret = objalloc(desc);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_DAWN_INSPECT
  // Register with global inspector for debugging

  CDevInspector *inspector = CDevInspector::getInst();
  inspector->registerProtoHandler(this);
#endif

  return OK;
}

int CProtoHandler::initAll()
{
  // Configure all registered protocols using base class
  int ret = CGenericHandler<CProtoCommon>::configureAll();
  if (ret != OK)
    {
      return ret;
    }

  // Bind IOs to protocols
  for (size_t i = 0; i < getObjectCount(); i++)
    {
      CProtoCommon *proto = getObjectAt(i);
      if (proto == nullptr)
        {
          continue;
        }

      // Collect IOs for this protocol
      for (auto const &[id, io] : proto->getIOMap())
        {
          CObject *resolvedIO = static_cast<CObject *>(getIO(id));
          proto->setObjectMapItem(id, resolvedIO);
        }
    }

  // Run one-time init after bindings are resolved
  return CGenericHandler<CProtoCommon>::initAll();
}

bool CProtoHandler::isObjectValid(SObjectId::UObjectId &obj) const
{
  return SObjectId::objectIsProto(obj);
};

CObject *CProtoHandler::getObject(const SObjectId::ObjectId id)
{
  SObjectId::UObjectId uid;
  uid.v = id;
  return reinterpret_cast<CObject *>(getProto(uid));
}

CProtoCommon *CProtoHandler::getProto(SObjectId::UObjectId &id) const
{
  return CGenericHandler<CProtoCommon>::getObjectById(id);
}
