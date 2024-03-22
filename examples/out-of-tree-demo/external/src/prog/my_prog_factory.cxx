// examples/out-of-tree-demo/external/src/prog/my_prog_factory.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "my_prog_factory.hxx"
#include "my_prog_dummy.hxx"

#include "dawn/debug.hxx"

using namespace oot_demo;

dawn::CProgCommon *CProgMyFactory::create(dawn::CDescObject &desc)
{
  switch (desc.getObjectCls())
    {
      case CProgMyDummy::PROG_CLASS_MY_DUMMY:
        DAWNINFO("CProgMyFactory: creating CProgMyDummy\n");
        return new CProgMyDummy(desc);

      default:
        return nullptr;
    }
}
