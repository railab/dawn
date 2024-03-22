// dawn/src/io/uuid.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/uuid.hxx"

#include <nuttx/board.h>

using namespace dawn;

int CIOUuid::init()
{
  if (getDtype() != SObjectId::DTYPE_UINT8)
    {
      DAWNERR("UUID requires DTYPE_UINT8\n");
      return -EINVAL;
    }

  return OK;
}

size_t CIOUuid::getDataDim() const
{
  return CONFIG_BOARDCTL_UNIQUEID_SIZE;
}

int CIOUuid::getDataImpl(IODataCmn &data, size_t len)
{
  uint8_t *uuid_buf;
  int ret;

  // No batch supported

  if (len != 1)
    {
      return -ENOTSUP;
    }

  // Get pointer to data buffer

  uuid_buf = static_cast<uint8_t *>(data.getDataPtr());

  // Get unique ID from board

  ret = board_uniqueid(uuid_buf);
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}

int CIOUuid::setDataImpl(IODataCmn &data)
{
  UNUSED(data);

  return -ENOTSUP;
}

size_t CIOUuid::getDataSize() const
{
  // Return size in bytes (CONFIG_BOARDCTL_UNIQUEID_SIZE bytes of uint8_t)

  return CONFIG_BOARDCTL_UNIQUEID_SIZE * sizeof(uint8_t);
}
