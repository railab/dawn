// dawn/src/system/factory.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/system/factory.hxx"

#ifdef CONFIG_DAWN_SYSTEM_LTE
#  include "dawn/system/lte.hxx"
#endif

using namespace dawn;

CSystemCommon *CSystemFactory::create(CDescObject &desc)
{
  switch (desc.getObjectCls())
    {
#ifdef CONFIG_DAWN_SYSTEM_LTE
      case CSystemCommon::SYSTEM_CLASS_LTE:
        return new CSystemLte(desc);
#endif

      default:
        return nullptr;
    }
}
