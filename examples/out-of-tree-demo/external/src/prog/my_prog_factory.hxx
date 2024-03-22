// examples/out-of-tree-demo/external/src/prog/my_prog_factory.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __DAWN_OOT_DEMO_PROG_MY_FACTORY_HXX
#define __DAWN_OOT_DEMO_PROG_MY_FACTORY_HXX

#include "dawn/prog/factory.hxx"

namespace oot_demo
{

class CProgMyFactory : public dawn::IProgFactory
{
public:
  CProgMyFactory(void)           = default;
  ~CProgMyFactory(void) override = default;

  dawn::CProgCommon *create(dawn::CDescObject &desc) override;
};

}  // namespace oot_demo

#endif  // __DAWN_OOT_DEMO_PROG_MY_FACTORY_HXX
