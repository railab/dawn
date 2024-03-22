// examples/out-of-tree-demo/external/src/io/my_io_factory.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __DAWN_OOT_DEMO_IO_MY_FACTORY_HXX
#define __DAWN_OOT_DEMO_IO_MY_FACTORY_HXX

#include "dawn/io/factory.hxx"

namespace oot_demo
{

/* User IO factory. The Dawn IO handler calls our factory first; if it returns
 * nullptr, the built-in factory takes over. So we only handle our own
 * user-class IDs and let everything else fall through.
 */

class CIOMyFactory : public dawn::IIOFactory
{
public:
  CIOMyFactory(void)           = default;
  ~CIOMyFactory(void) override = default;

  dawn::CIOCommon *create(dawn::CDescObject &desc) override;
};

}  // namespace oot_demo

#endif  // __DAWN_OOT_DEMO_IO_MY_FACTORY_HXX
