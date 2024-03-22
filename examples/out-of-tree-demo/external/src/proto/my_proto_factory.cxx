// examples/out-of-tree-demo/external/src/proto/my_proto_factory.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "my_proto_factory.hxx"
#include "my_proto_dummy.hxx"

#include "dawn/debug.hxx"

using namespace oot_demo;

dawn::CProtoCommon *CProtoMyFactory::create(dawn::CDescObject &desc)
{
  switch (desc.getObjectCls())
    {
      case CProtoMyDummy::PROTO_CLASS_MY_DUMMY:
        DAWNINFO("CProtoMyFactory: creating CProtoMyDummy\n");
        return new CProtoMyDummy(desc);

      default:
        return nullptr;
    }
}
