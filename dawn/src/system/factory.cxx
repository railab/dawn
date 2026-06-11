// dawn/src/system/factory.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/system/factory.hxx"

#ifdef CONFIG_DAWN_SYSTEM_LTE
#  include "dawn/system/lte.hxx"
#endif

#ifdef CONFIG_DAWN_SYSTEM_GNSS
#  include "dawn/system/gnss.hxx"
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

#ifdef CONFIG_DAWN_SYSTEM_GNSS
      case CSystemCommon::SYSTEM_CLASS_GNSS:
        return new CSystemGnss(desc);
#endif

      default:
        return nullptr;
    }
}
