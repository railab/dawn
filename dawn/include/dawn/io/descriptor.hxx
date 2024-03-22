// dawn/include/dawn/io/descriptor.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/dev/descriptor.hxx"
#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Device descriptor I/O access.
 *
 * Provides read/write access to device descriptors.
 */

class CIODescriptor : public CIOCommon
{
public:
  enum
  {
    IO_DESCRIPTOR_CFG_FIRST = 0,
    IO_DESCRIPTOR_CFG_LAST = 31
  };

  explicit CIODescriptor(CDescObject &desc)
    : CIOCommon(desc)
  {
  }

  ~CIODescriptor() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "descriptor";
  }
#endif

  int configure() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;
  int setDataAtImpl(IODataCmn &data, size_t offset) override;
  int getDataAtImpl(IODataCmn &data, size_t len, size_t offset) override;

#ifdef CONFIG_DAWN_IO_NOTIFY
  int getFd() const override;
#endif

  size_t getDataSize() const override;
  size_t getDataDim() const override;

  bool isRead() const override
  {
    return true;
  };

  bool isWrite() const override
  {
    return getCmnDevno() != 0;
  };

  bool isNotify() const override
  {
    return true;
  };

  bool isBatch() const override
  {
    return false;
  };

  bool isSeekable() const override
  {
    return true;
  }

  using ObjectIdHelper =
    CIOCommon::IOObjectIdHelperNoTS<IO_CLASS_DESCRIPTOR, SObjectId::DTYPE_BLOCK>;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return ObjectIdHelper::create(inst);
  }

private:
  CDevDescriptor::SDescriptorReg regDesc = {}; ///< Registered descriptor storage and registry.

  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
