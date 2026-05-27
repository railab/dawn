// dawn/tests/mocks/fake_iowriteonlymock.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstring>

#include "dawn/io/common.hxx"

using namespace dawn;

class CIOWriteOnlyScalarMock : public CIOCommon
{
public:
  static constexpr uint16_t IO_CLASS_TEST_WRITEONLY = 501;
  using ObjectIdHelper =
    CIOCommon::IOObjectIdHelperNoTS<IO_CLASS_TEST_WRITEONLY, SObjectId::DTYPE_UINT16>;

  explicit CIOWriteOnlyScalarMock(CDescObject &desc)
    : CIOCommon(desc)
    , lastValue(0)
  {
  }

  ~CIOWriteOnlyScalarMock() override = default;

  static constexpr SObjectId::ObjectId objectId(SObjectId::EObjectDataType dtype, uint16_t inst)
  {
    return ObjectIdHelper::create(dtype, inst);
  }

  int init() override
  {
    return getDataSize() > 0 ? OK : -EINVAL;
  }

  int setDataImpl(IODataCmn &data) override
  {
    if (data.getItems() < 1)
      {
        return -EINVAL;
      }

    std::memcpy(&lastValue, data.getDataPtr(), getDataSize());
    return OK;
  }

  size_t getDataSize() const override
  {
    switch (getDtype())
      {
        case SObjectId::DTYPE_BOOL:
          return sizeof(uint8_t);

        case SObjectId::DTYPE_UINT16:
          return sizeof(uint16_t);

        case SObjectId::DTYPE_UINT32:
          return sizeof(uint32_t);

        case SObjectId::DTYPE_UINT64:
          return sizeof(uint64_t);

        default:
          return 0;
      }
  }

  size_t getDataDim() const override
  {
    return 1;
  }

  bool isRead() const override
  {
    return false;
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

  uint64_t getLastValue() const
  {
    return lastValue;
  }

private:
  uint64_t lastValue;
};
