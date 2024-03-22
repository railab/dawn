// dawn/include/dawn/io/descselector.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/descriptor.hxx"
#include "dawn/dev/descriptor.hxx"
#include "dawn/dev/descswitch.hxx"
#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/** @brief Runtime descriptor slot selector I/O. */

class CIODescSelector : public CIOCommon
{
public:
  explicit CIODescSelector(CDescObject &desc)
    : CIOCommon(desc)
  {
  }

  ~CIODescSelector() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "descselector";
  }
#endif

  int configure() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;

  size_t getDataSize() const override
  {
    return sizeof(uint32_t);
  }

  size_t getDataDim() const override
  {
    return 1;
  }

  bool isRead() const override
  {
    return true;
  }

  bool isWrite() const override
  {
    return true;
  }

  bool isNotify() const override
  {
    return false;
  }

  bool isBatch() const override
  {
    return false;
  }

  using ObjectIdHelper =
    CIOCommon::IOObjectIdHelperNoTS<IO_CLASS_DESC_SELECTOR, SObjectId::DTYPE_UINT32>;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return ObjectIdHelper::create(inst);
  }

private:
  int configureDesc(const CDescObject &desc);
};

} // Namespace dawn
