// examples/out-of-tree-demo/external/src/io/my_io_dummy.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __DAWN_OOT_DEMO_IO_MY_IO_DUMMY_HXX
#define __DAWN_OOT_DEMO_IO_MY_IO_DUMMY_HXX

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"

namespace oot_demo
{

/* User-defined IO class. The IO_CLASS_USER slot (501) is reserved by Dawn for
 * out-of-tree user classes, see dawn/include/dawn/io/common.hxx.
 */

class CIOMyDummy : public dawn::CIOCommon
{
public:
  static constexpr uint16_t IO_CLASS_MY_DUMMY = dawn::CIOCommon::IO_CLASS_USER;

  enum
  {
    IO_MY_DUMMY_CFG_FIRST   = 0,
    IO_MY_DUMMY_CFG_INITVAL = 1,
    IO_MY_DUMMY_CFG_LAST    = 31,
  };

  explicit CIOMyDummy(dawn::CDescObject &desc)
    : dawn::CIOCommon(desc)
    , value(0xCAFEBABE)
  {
  }

  ~CIOMyDummy(void) override = default;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr(void) const override
  {
    return "mydummy";
  }
#endif

  int configure(void) override;

  int init(void) override
  {
    return OK;
  }

  int deinit(void) override
  {
    return OK;
  }

  int getDataImpl(dawn::IODataCmn &data, size_t len) override;
  int setDataImpl(dawn::IODataCmn &data) override;

  size_t getDataSize(void) const override
  {
    return sizeof(uint32_t);
  }

  size_t getDataDim(void) const override
  {
    return 1;
  }

  bool isRead(void) const override   { return true;  }
  bool isWrite(void) const override  { return true;  }
  bool isNotify(void) const override { return false; }
  bool isBatch(void) const override  { return false; }

  using ObjectIdHelper = dawn::CIOCommon::IOObjectIdHelperNoTS<
      IO_CLASS_MY_DUMMY,
      dawn::SObjectId::DTYPE_UINT32>;

  constexpr static dawn::SObjectId::ObjectId objectId(uint16_t inst)
  {
    return ObjectIdHelper::create(inst);
  }

  /**
   * @brief Initial-value config helper.
   *
   * Emits a single uint32 word into the descriptor that this IO uses
   * as its starting `value`.
   */
  constexpr static dawn::SObjectCfg::ObjectCfgId cfgIdInitval(void)
  {
    return dawn::SObjectCfg::objectCfg(
        dawn::SObjectId::OBJTYPE_IO,
        IO_CLASS_MY_DUMMY,
        dawn::SObjectId::DTYPE_UINT32,
        false,
        1,
        IO_MY_DUMMY_CFG_INITVAL);
  }

private:
  uint32_t value;
};

}  // namespace oot_demo

#endif  // __DAWN_OOT_DEMO_IO_MY_IO_DUMMY_HXX
