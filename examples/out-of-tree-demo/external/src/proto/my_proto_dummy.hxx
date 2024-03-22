// examples/out-of-tree-demo/external/src/proto/my_proto_dummy.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __DAWN_OOT_DEMO_PROTO_MY_PROTO_DUMMY_HXX
#define __DAWN_OOT_DEMO_PROTO_MY_PROTO_DUMMY_HXX

#include "dawn/proto/common.hxx"
#include "dawn/porting/config.hxx"

namespace oot_demo
{

/* User-defined PROTO class. Bound to one or more IOs through the standard
 * IOBIND configuration; serves as a placeholder for the demo.
 */

class CProtoMyDummy : public dawn::CProtoCommon
{
public:
  static constexpr uint16_t PROTO_CLASS_MY_DUMMY =
      dawn::CProtoCommon::PROTO_CLASS_USER;

  enum
  {
    PROTO_MY_DUMMY_CFG_FIRST  = 0,
    PROTO_MY_DUMMY_CFG_IOBIND = 1,
    PROTO_MY_DUMMY_CFG_LAST   = 31
  };

  explicit CProtoMyDummy(dawn::CDescObject &desc)
    : dawn::CProtoCommon(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr(void) const override
  {
    return "proto_mydummy";
  }
#endif

  int configure(void) override;

  constexpr static dawn::SObjectId::ObjectId objectId(uint16_t inst)
  {
    return dawn::SObjectId::objectId(
        dawn::SObjectId::OBJTYPE_PROTO,
        PROTO_CLASS_MY_DUMMY,
        dawn::SObjectId::DTYPE_ANY,
        0,
        inst);
  }

  constexpr static dawn::SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 0)
  {
    return dawn::SObjectCfg::objectCfg(
        dawn::SObjectId::OBJTYPE_PROTO,
        PROTO_CLASS_MY_DUMMY,
        dawn::SObjectId::DTYPE_ANY,
        false,
        size,
        PROTO_MY_DUMMY_CFG_IOBIND);
  }
};

}  // namespace oot_demo

#endif  // __DAWN_OOT_DEMO_PROTO_MY_PROTO_DUMMY_HXX
