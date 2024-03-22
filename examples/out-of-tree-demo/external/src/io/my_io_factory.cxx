// examples/out-of-tree-demo/external/src/io/my_io_factory.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "my_io_factory.hxx"
#include "my_io_dummy.hxx"

#include "dawn/debug.hxx"

using namespace oot_demo;

dawn::CIOCommon *CIOMyFactory::create(dawn::CDescObject &desc)
{
  switch (desc.getObjectCls())
    {
      case CIOMyDummy::IO_CLASS_MY_DUMMY:
        DAWNINFO("CIOMyFactory: creating CIOMyDummy\n");
        return new CIOMyDummy(desc);

      default:
        return nullptr;
    }
}
