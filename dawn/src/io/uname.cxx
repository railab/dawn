// dawn/src/io/uname.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/uname.hxx"

#include <sys/utsname.h>

using namespace dawn;

int CIOUname::init()
{
  if (getCls() == IO_CLASS_SYSTEM_HOSTNAME && getDtype() != SObjectId::DTYPE_CHAR)
    {
      DAWNERR("HOSTNAME requires DTYPE_CHAR\n");
      return -EINVAL;
    }

  return OK;
}

size_t CIOUname::getDataDim() const
{
  return dim;
}

size_t CIOUname::getDataDim(uint16_t cls) const
{
  switch (cls)
    {
      case IO_CLASS_SYSTEM_HOSTNAME:
        {
          return HOST_NAME_MAX;
        }

      default:
        {
          DAWNERR("Invalid uname class %d\n", cls);
          return 0;
        }
    }
}

int CIOUname::getDataImpl(IODataCmn &data, size_t len)
{
  struct utsname name_;
  int ret;

  // No batch supported

  if (len != 1)
    {
      return -ENOTSUP;
    }

  /* Get system info */

  ret = uname(&name_);
  if (ret < 0)
    {
      DAWNERR("uname failed %d\n", ret);
      return ret;
    }

  char *tmp = static_cast<char *>(data.getDataPtr());
  size_t maxlen = data.getDataSize();

  // Copy node name to buffer

  std::memset(tmp, 0, maxlen);
  if (maxlen > 0)
    {
      std::strncpy(tmp, name_.nodename, maxlen - 1);
    }

  return OK;
}

int CIOUname::setDataImpl(IODataCmn &data)
{
  UNUSED(data);

  return -ENOTSUP;
}

size_t CIOUname::getDataSize() const
{
  switch (getCls())
    {
      case IO_CLASS_SYSTEM_HOSTNAME:
        {
          return HOST_NAME_MAX * sizeof(char);
        }

      default:
        {
          return 0;
        }
    }
}
