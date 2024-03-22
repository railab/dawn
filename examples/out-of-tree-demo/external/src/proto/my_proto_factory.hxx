// examples/out-of-tree-demo/external/src/proto/my_proto_factory.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __DAWN_OOT_DEMO_PROTO_MY_FACTORY_HXX
#define __DAWN_OOT_DEMO_PROTO_MY_FACTORY_HXX

#include "dawn/proto/factory.hxx"

namespace oot_demo
{

class CProtoMyFactory : public dawn::IProtoFactory
{
public:
  CProtoMyFactory(void)           = default;
  ~CProtoMyFactory(void) override = default;

  dawn::CProtoCommon *create(dawn::CDescObject &desc) override;
};

}  // namespace oot_demo

#endif  // __DAWN_OOT_DEMO_PROTO_MY_FACTORY_HXX
