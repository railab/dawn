// dawn/src/io/handler.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/handler.hxx"

#include <fixedmath.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <functional>

#include <errno.h>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"

#ifdef CONFIG_DAWN_IO_CONFIG
#  include "dawn/io/config.hxx"
#endif

#ifdef CONFIG_DAWN_IO_CONTROL
#  include "dawn/io/control.hxx"
#endif

#ifdef CONFIG_DAWN_IO_TRIGGER
#  include "dawn/io/trigger.hxx"
#endif

#ifdef CONFIG_DAWN_IO_DESCRIPTOR
#  include "dawn/io/descriptor.hxx"
#endif

#ifdef CONFIG_DAWN_INSPECT
#  include "dawn/dev/inspector.hxx"
#endif

using namespace dawn;

void CIOHandler::objallocPriv(CHandler &obj, CDescObject &desc)
{
  // Allocate object

  if (SObjectId::objectIsIO(desc.getObjectId()))
    {
      CIOHandler &t = static_cast<CIOHandler &>(obj);
      CIOCommon *tmp = nullptr;

      tmp = t.create(desc);
      if (tmp == nullptr)
        {
          DAWNERR("Failed to create IO object 0x%" PRIx32 "\n", desc.getObjectIdV());
          t.allocError = -ENOMEM;
          return;
        }

      DAWNINFO("created IO: 0x%" PRIx32 " %p\n", desc.getObjectIdV(), tmp);

      if (t.registerObject(tmp) < 0)
        {
          return;
        }
    }
}

CIOCommon *CIOHandler::create(CDescObject &desc)
{
  CIOCommon *ret = nullptr;

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

int CIOHandler::objalloc(CDescriptor &desc)
{
  return CGenericHandler<CIOCommon>::objalloc(desc, objallocPriv);
}

int CIOHandler::init(CDescriptor &desc, IIOFactory *f)
{
  int ret;

  // Set user factory

  userFactory = f;

  // Allocate objects from descriptor

  ret = objalloc(desc);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_DAWN_INSPECT
  // Register with global inspector for debugging

  CDevInspector *inspector = CDevInspector::getInst();
  inspector->registerIOHandler(this);
#endif

  return OK;
}

void CIOHandler::onInitObject(CIOCommon *io)
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  notifyMgr.regIO(io);
#endif
}

int CIOHandler::startAll()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  // Start all notifier instances
  notifyMgr.start();
#endif

  // Call base class to start all IOs
  return CGenericHandler<CIOCommon>::startAll();
}

int CIOHandler::initAll()
{
  int ret;

  // Configure IO objects (descriptor parsing / validation)
  ret = CGenericHandler<CIOCommon>::configureAll();
  if (ret != OK)
    {
      return ret;
    }

  // One-time IO init (allocations / fd open)
  ret = CGenericHandler<CIOCommon>::initAll();
  if (ret != OK)
    {
      return ret;
    }

  return OK;
}

int CIOHandler::stopAll()
{
  // Call base class to stop all IOs
  int ret = CGenericHandler<CIOCommon>::stopAll();

#ifdef CONFIG_DAWN_IO_NOTIFY
  // Stop all notifier instances
  notifyMgr.stop();
#endif

  return ret;
}

bool CIOHandler::isObjectValid(SObjectId::UObjectId &obj) const
{
  return SObjectId::objectIsIO(obj);
}

CObject *CIOHandler::getObject(const SObjectId::ObjectId id)
{
  SObjectId::UObjectId uid;
  uid.v = id;
  return static_cast<CObject *>(getIO(uid));
}

CIOCommon *CIOHandler::getIO(SObjectId::UObjectId &id) const
{
  return CGenericHandler<CIOCommon>::getObjectById(id);
}

#if defined(CONFIG_DAWN_IO_CONFIG) || defined(CONFIG_DAWN_IO_CONTROL) || \
  defined(CONFIG_DAWN_IO_TRIGGER)
static int resolveObjectById(SObjectId::ObjectId id,
                             IHandler &io,
                             IHandler &prog,
                             IHandler &prot,
                             CObject **out)
{
  CObject *obj;

  switch (SObjectId::objectIdGetType(id))
    {
      case SObjectId::OBJTYPE_IO:
        {
          obj = io.getObject(id);
          break;
        }

      case SObjectId::OBJTYPE_PROTO:
        {
          obj = prot.getObject(id);
          break;
        }

      case SObjectId::OBJTYPE_PROG:
        {
          obj = prog.getObject(id);
          break;
        }

      default:
        {
          DAWNERR("invalid object type for ID 0x%" PRIx32 "\n", id);
          return -EINVAL;
        }
    }

  if (obj == nullptr)
    {
      DAWNERR("object not found for ID 0x%" PRIx32 "\n", id);
      return -ENOENT;
    }

  *out = obj;
  return OK;
}
#endif

#if defined(CONFIG_DAWN_IO_CONFIG) || defined(CONFIG_DAWN_IO_CONTROL) || \
  defined(CONFIG_DAWN_IO_TRIGGER)
static int bindGeneric(IHandler &io,
                       IHandler &prog,
                       IHandler &prot,
                       const std::vector<SObjectId::ObjectId> &ids,
                       const std::function<int(CObject *, SObjectId::ObjectId)> &bind)
{
  for (auto const &id : ids)
    {
      CObject *obj;
      int ret;

      ret = resolveObjectById(id, io, prog, prot, &obj);
      if (ret < 0)
        {
          return ret;
        }

      ret = bind(obj, id);
      if (ret < 0)
        {
          DAWNERR("bind failed for ID 0x%" PRIx32 " (error %d)\n", id, ret);
          return ret;
        }
    }

  return OK;
}
#endif

int CIOHandler::bindObjects(IHandler &io, IHandler &prog, IHandler &prot)
{
  UNUSED(io);
  UNUSED(prog);
  UNUSED(prot);

#if defined(CONFIG_DAWN_IO_CONFIG) || defined(CONFIG_DAWN_IO_CONTROL) || \
  defined(CONFIG_DAWN_IO_TRIGGER)
  for (CIOCommon *tmp : objects)
    {
#  ifdef CONFIG_DAWN_IO_CONFIG
      if (tmp->getCls() == CIOCommon::IO_CLASS_CONFIG)
        {
          CIOConfig *cfg = static_cast<CIOConfig *>(tmp);
          std::vector<SObjectId::ObjectId> ids;

          ids.reserve(cfg->map.size());
          for (auto const &[id, _] : cfg->map)
            {
              ids.push_back(id);
            }

          int ret =
            bindGeneric(io,
                        prog,
                        prot,
                        ids,
                        [cfg](CObject *o, SObjectId::ObjectId id) { return cfg->bind(o, id); });
          if (ret < 0)
            {
              return ret;
            }
        }
#  endif // CONFIG_DAWN_IO_CONFIG

#  ifdef CONFIG_DAWN_IO_CONTROL
      if (tmp->getCls() == CIOCommon::IO_CLASS_CONTROL)
        {
          CIOControl *ctrl = static_cast<CIOControl *>(tmp);

          int ret = bindGeneric(io,
                                prog,
                                prot,
                                ctrl->ids,
                                [ctrl](CObject *o, SObjectId::ObjectId) { return ctrl->bind(o); });
          if (ret < 0)
            {
              return ret;
            }
        }
#  endif // CONFIG_DAWN_IO_CONTROL

#  ifdef CONFIG_DAWN_IO_TRIGGER
      if (tmp->getCls() == CIOCommon::IO_CLASS_TRIGGER)
        {
          CIOTrigger *trig = static_cast<CIOTrigger *>(tmp);

          int ret = bindGeneric(io,
                                prog,
                                prot,
                                trig->ids,
                                [trig](CObject *o, SObjectId::ObjectId) { return trig->bind(o); });
          if (ret < 0)
            {
              return ret;
            }
        }
#  endif // CONFIG_DAWN_IO_TRIGGER
    }
#endif

  return OK;
}
