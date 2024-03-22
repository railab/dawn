// dawn/tests/mocks/fake_ioseekmock.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <errno.h>
#include <string.h>

#include "dawn/io/common.hxx"

using namespace dawn;

//***************************************************************************
// Description: seekable IO mock supports offset reads and writes.
//***************************************************************************

class CIOSeekableMock : public CIOCommon
{
public:
  explicit CIOSeekableMock(CDescObject &desc)
    : CIOCommon(desc)
  {
    memset(buf, 0, sizeof(buf));
  }

  int configure() override
  {
    return OK;
  }

  int deinit() override
  {
    return OK;
  }

  int getDataImpl(IODataCmn &data, size_t len) override
  {
    if (len != 1 || data.getDataSize() < sizeof(buf))
      {
        return -EINVAL;
      }

    memcpy(data.getDataPtr(0), buf, sizeof(buf));
    return OK;
  }

  int setDataImpl(IODataCmn &data) override
  {
    (void)data;
    return -ENOTSUP;
  }

  int getDataAtImpl(IODataCmn &data, size_t len, size_t offset) override
  {
    size_t avail;
    size_t to_copy;

    if (len != 1 || offset >= sizeof(buf))
      {
        return -EINVAL;
      }

    avail = sizeof(buf) - offset;
    to_copy = data.getDataSize() < avail ? data.getDataSize() : avail;
    memcpy(data.getDataPtr(0), &buf[offset], to_copy);
    return OK;
  }

  int setDataAtImpl(IODataCmn &data, size_t offset) override
  {
    if (offset > sizeof(buf) || data.getDataSize() > sizeof(buf) - offset)
      {
        return -EINVAL;
      }

    memcpy(&buf[offset], data.getDataPtr(0), data.getDataSize());
    return OK;
  }

  size_t getDataSize() const override
  {
    return sizeof(buf);
  }

  size_t getDataDim() const override
  {
    return sizeof(buf);
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

  bool isSeekable() const override
  {
    return true;
  }

private:
  uint8_t buf[32];
};
