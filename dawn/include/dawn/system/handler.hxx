// dawn/include/dawn/system/handler.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/generic_handler.hxx"
#include "dawn/system/common.hxx"
#include "dawn/system/factory.hxx"

namespace dawn
{
/** @brief Handler for SYSTEM objects (OBJTYPE_ANY, cls != 0). */

class CSystemHandler : public CGenericHandler<CSystemCommon>
{
public:
  CSystemHandler()
    : userFactory(nullptr)
  {
  }

  int init(CDescriptor &desc, ISystemFactory *f);

  int initAll() override
  {
    int ret = CGenericHandler<CSystemCommon>::configureAll();
    if (ret != OK)
      {
        return ret;
      }

    return CGenericHandler<CSystemCommon>::initAll();
  }

  int deinitAll() override
  {
    return CGenericHandler<CSystemCommon>::deinitAll();
  }

  int startAll() override
  {
    return CGenericHandler<CSystemCommon>::startAll();
  }

  int stopAll() override
  {
    return CGenericHandler<CSystemCommon>::stopAll();
  }

  bool hasThread() const override
  {
    return CGenericHandler<CSystemCommon>::hasThread();
  }

  bool isObjectValid(SObjectId::UObjectId &obj) const override
  {
    return SObjectId::objectIsSystem(obj);
  }

  CObject *getObject(const SObjectId::ObjectId id) override
  {
    SObjectId::UObjectId uid;
    uid.v = id;
    return CGenericHandler<CSystemCommon>::getObjectById(uid);
  }

  CSystemCommon *getSystem(SObjectId::UObjectId &id) const
  {
    return CGenericHandler<CSystemCommon>::getObjectById(id);
  }

protected:
  const char *getObjectTypeName() const override
  {
    return "SYSTEM";
  }

private:
  CSystemFactory factory;
  ISystemFactory *userFactory;

  static void objallocPriv(CHandler &obj, CDescObject &desc);
  int objalloc(CDescriptor &desc);
  CSystemCommon *create(CDescObject &desc);
};
} // Namespace dawn
