// dawn/src/system/handler.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/system/handler.hxx"

#include "dawn/debug.hxx"

using namespace dawn;

void CSystemHandler::objallocPriv(CHandler &obj, CDescObject &desc)
{
  if (SObjectId::objectIsSystem(desc.getObjectId()))
    {
      CSystemHandler &t = static_cast<CSystemHandler &>(obj);
      CSystemCommon *tmp = t.create(desc);

      if (tmp == nullptr)
        {
          DAWNERR("Failed to create SYSTEM object 0x%" PRIx32 "\n", desc.getObjectIdV());
          t.allocError = -ENOMEM;
          return;
        }

      DAWNINFO("created SYSTEM: 0x%" PRIx32 " %p\n", desc.getObjectIdV(), tmp);
      t.registerObject(tmp);
    }
}

CSystemCommon *CSystemHandler::create(CDescObject &desc)
{
  if (userFactory != nullptr)
    {
      CSystemCommon *ret = userFactory->create(desc);
      if (ret != nullptr)
        {
          return ret;
        }
    }

  return factory.create(desc);
}

int CSystemHandler::objalloc(CDescriptor &desc)
{
  return CGenericHandler<CSystemCommon>::objalloc(desc, objallocPriv);
}

int CSystemHandler::init(CDescriptor &desc, ISystemFactory *f)
{
  userFactory = f;
  return objalloc(desc);
}
