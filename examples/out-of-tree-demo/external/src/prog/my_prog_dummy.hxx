// examples/out-of-tree-demo/external/src/prog/my_prog_dummy.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __DAWN_OOT_DEMO_PROG_MY_PROG_DUMMY_HXX
#define __DAWN_OOT_DEMO_PROG_MY_PROG_DUMMY_HXX

#include "dawn/prog/common.hxx"
#include "dawn/porting/config.hxx"

namespace oot_demo
{

/* User-defined PROG class. Bound to one or more IOs through the standard
 * IOBIND configuration; serves as a placeholder for the demo.
 */

class CProgMyDummy : public dawn::CProgCommon
{
public:
  static constexpr uint16_t PROG_CLASS_MY_DUMMY =
      dawn::CProgCommon::PROG_CLASS_USER;

  enum
  {
    PROG_MY_DUMMY_CFG_FIRST  = 0,
    PROG_MY_DUMMY_CFG_IOBIND = 1,
    PROG_MY_DUMMY_CFG_TAG    = 2,
    PROG_MY_DUMMY_CFG_LAST   = 31
  };

  explicit CProgMyDummy(dawn::CDescObject &desc)
    : dawn::CProgCommon(desc)
    , tag(0)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr(void) const override
  {
    return "prog_mydummy";
  }
#endif

  int configure(void) override;

  constexpr static dawn::SObjectId::ObjectId objectId(uint16_t inst)
  {
    return dawn::SObjectId::objectId(
        dawn::SObjectId::OBJTYPE_PROG,
        PROG_CLASS_MY_DUMMY,
        dawn::SObjectId::DTYPE_ANY,
        0,
        inst);
  }

  constexpr static dawn::SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 0)
  {
    return dawn::SObjectCfg::objectCfg(
        dawn::SObjectId::OBJTYPE_PROG,
        PROG_CLASS_MY_DUMMY,
        dawn::SObjectId::DTYPE_ANY,
        false,
        size,
        PROG_MY_DUMMY_CFG_IOBIND);
  }

  /**
   * @brief Tag config helper.
   *
   * Emits a single uint32 word into the descriptor that the program
   * stores as its tag. Demonstrates ConfigField-driven custom config
   * for PROG types.
   */
  constexpr static dawn::SObjectCfg::ObjectCfgId cfgIdTag(void)
  {
    return dawn::SObjectCfg::objectCfg(
        dawn::SObjectId::OBJTYPE_PROG,
        PROG_CLASS_MY_DUMMY,
        dawn::SObjectId::DTYPE_UINT32,
        false,
        1,
        PROG_MY_DUMMY_CFG_TAG);
  }

private:
  uint32_t tag;
};

}  // namespace oot_demo

#endif  // __DAWN_OOT_DEMO_PROG_MY_PROG_DUMMY_HXX
