// dawn/src/prog/handler.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/handler.hxx"

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"

#ifdef CONFIG_DAWN_INSPECT
#  include "dawn/dev/inspector.hxx"
#endif

using namespace dawn;

void CProgHandler::objallocPriv(CHandler &obj, CDescObject &desc)
{
  // Allocate object

  if (SObjectId::objectIsProg(desc.getObjectId()))
    {
      CProgHandler &t = static_cast<CProgHandler &>(obj);
      CProgCommon *tmp = nullptr;

      tmp = t.create(desc);
      if (tmp == nullptr)
        {
          DAWNERR("Failed to create PROG object 0x%" PRIx32 "\n", desc.getObjectIdV());
          t.allocError = -ENOMEM;
          return;
        }

      DAWNINFO("created PROG: 0x%" PRIx32 " %p\n", desc.getObjectIdV(), tmp);
      if (t.registerObject(tmp) < 0)
        {
          return;
        }
    }
}

CProgCommon *CProgHandler::create(CDescObject &desc)
{
  // User factory has priority

  if (userFactory != nullptr)
    {
      CProgCommon *ret = nullptr;

      ret = userFactory->create(desc);
      if (ret != nullptr)
        {
          return ret;
        }
    }

  return factory.create(desc);
}

CIOCommon *CProgHandler::getIO(SObjectId::ObjectId id)
{
  SObjectId::UObjectId uid;
  uid.v = id;
  return ioHandler->getIO(uid);
}

int CProgHandler::objalloc(CDescriptor &desc)
{
  return CGenericHandler<CProgCommon>::objalloc(desc, objallocPriv);
}

int CProgHandler::init(CDescriptor &desc, CIOHandler *io, IProgFactory *f)
{
  int ret;

  // Set user factory
  userFactory = f;

  // Connect handler
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
  inspector->registerProgHandler(this);
#endif

  return OK;
}

int CProgHandler::initAll()
{
  // Configure all registered programs using base class
  int ret = CGenericHandler<CProgCommon>::configureAll();
  if (ret != OK)
    {
      return ret;
    }

  // Bind IOs to programs
  for (size_t i = 0; i < getObjectCount(); i++)
    {
      CProgCommon *prog = getObjectAt(i);
      if (prog == nullptr)
        {
          continue;
        }

      // Collect IOs for this program
      for (auto const &[id, io] : prog->getIOMap())
        {
          CObject *resolvedIO = static_cast<CObject *>(getIO(id));
          prog->setObjectMapItem(id, resolvedIO);
        }
    }

  // Run one-time init after bindings are resolved
  return CGenericHandler<CProgCommon>::initAll();
}

bool CProgHandler::isObjectValid(SObjectId::UObjectId &obj) const
{
  return SObjectId::objectIsProg(obj);
};

CObject *CProgHandler::getObject(const SObjectId::ObjectId id)
{
  SObjectId::UObjectId uid;
  uid.v = id;
  return reinterpret_cast<CObject *>(getProg(uid));
}

CProgCommon *CProgHandler::getProg(SObjectId::UObjectId &id) const
{
  return CGenericHandler<CProgCommon>::getObjectById(id);
}
