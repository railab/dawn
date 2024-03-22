// dawn/src/io/boardctl.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/boardctl.hxx"

#include <sys/boardctl.h>

using namespace dawn;

#ifdef CONFIG_BOARDCTL_RESET
#endif
#ifdef CONFIG_BOARDCTL_RESET_CAUSE
#endif
#ifdef CONFIG_BOARDCTL_POWEROFF
#endif

size_t CIOBoardctl::getDataDim() const
{
  return dim;
}

size_t CIOBoardctl::getDataDim(uint16_t cls) const
{
  switch (cls)
    {
#ifdef CONFIG_BOARDCTL_RESET_CAUSE
      case IO_CLASS_SYSTEM_RESETCAUSE:
        {
          return 2;
        }
#endif

#ifdef CONFIG_BOARDCTL_RESET
      case IO_CLASS_SYSTEM_RESET:
        {
          return 1;
        }
#endif

#ifdef CONFIG_BOARDCTL_POWEROFF
      case IO_CLASS_SYSTEM_POWEROFF:
        {
          return 1;
        }
#endif

      default:
        {
          DAWNERR("Invalid boardctl class %d\n", cls);
          return 0;
        }
    }
}

int CIOBoardctl::getDataImpl(IODataCmn &data, size_t len)
{
  int ret = OK;

  // Read not supported

  if (!isRead())
    {
      return -ENOTSUP;
    }

  // No batch supported

  if (len != 1)
    {
      return -ENOTSUP;
    }

  // Handle request

  switch (getCls())
    {
#ifdef CONFIG_BOARDCTL_RESET_CAUSE
      case IO_CLASS_SYSTEM_RESETCAUSE:
        {
          uint32_t *tmp = static_cast<uint32_t *>(data.getDataPtr());
          struct boardioc_reset_cause_s cause;

          std::memset(&cause, 0, sizeof(struct boardioc_reset_cause_s));

          ret = boardctl(BOARDIOC_RESET_CAUSE, (uintptr_t)&cause);
          if (ret < 0)
            {
              DAWNERR("BOARDIOC_RESET_CAUSE failed %d\n", ret);
              return ret;
            }

          // Copy data

          tmp[0] = cause.cause;
          tmp[1] = cause.flag;

          break;
        }
#endif

      default:
        {
          return -EINVAL;
        }
    }

  return ret;
}

int CIOBoardctl::setDataImpl(IODataCmn &data)
{
  UNUSED(data);

  // Write not supported

  if (!isWrite())
    {
      return -ENOTSUP;
    }

  // Handle request

  switch (getCls())
    {
#ifdef CONFIG_BOARDCTL_RESET
      case IO_CLASS_SYSTEM_RESET:
        {
          int32_t *outvalue = static_cast<int32_t *>(data.getDataPtr());
          return boardctl(BOARDIOC_RESET, (int)*outvalue);
        }
#endif

#ifdef CONFIG_BOARDCTL_POWEROFF
      case IO_CLASS_SYSTEM_POWEROFF:
        {
          int32_t *outvalue = static_cast<int32_t *>(data.getDataPtr());
          return boardctl(BOARDIOC_POWEROFF, (int)*outvalue);
        }
#endif

      default:
        {
          return -EINVAL;
        }
    }
}

size_t CIOBoardctl::getDataSize() const
{
  switch (getCls())
    {
#ifdef CONFIG_BOARDCTL_RESET
      case IO_CLASS_SYSTEM_RESET:
        {
          return sizeof(int32_t) * dim;
        }
#endif

#ifdef CONFIG_BOARDCTL_RESET_CAUSE
      case IO_CLASS_SYSTEM_RESETCAUSE:
        {
          return sizeof(uint32_t) * dim;
        }
#endif

#ifdef CONFIG_BOARDCTL_POWEROFF
      case IO_CLASS_SYSTEM_POWEROFF:
        {
          return sizeof(int32_t) * dim;
        }
#endif

      default:
        {
          return 0;
        }
    }
}

bool CIOBoardctl::isRead() const
{
  switch (getCls())
    {
#ifdef CONFIG_BOARDCTL_RESET
      case IO_CLASS_SYSTEM_RESET:
        {
          return false;
        }
#endif

#ifdef CONFIG_BOARDCTL_RESET_CAUSE
      case IO_CLASS_SYSTEM_RESETCAUSE:
        {
          return true;
        }
#endif

#ifdef CONFIG_BOARDCTL_POWEROFF
      case IO_CLASS_SYSTEM_POWEROFF:
        {
          return false;
        }
#endif

      default:
        {
          DAWNERR("Unknown boardctl class %d for isRead\n", getCls());
          return false;
        }
    }
};

bool CIOBoardctl::isWrite() const
{
  switch (getCls())
    {
#ifdef CONFIG_BOARDCTL_RESET
      case IO_CLASS_SYSTEM_RESET:
        {
          return true;
        }
#endif

#ifdef CONFIG_BOARDCTL_RESET_CAUSE
      case IO_CLASS_SYSTEM_RESETCAUSE:
        {
          return false;
        }
#endif

#ifdef CONFIG_BOARDCTL_POWEROFF
      case IO_CLASS_SYSTEM_POWEROFF:
        {
          return true;
        }
#endif

      default:
        {
          DAWNERR("Unknown boardctl class %d for isWrite\n", getCls());
          return false;
        }
    }
};
