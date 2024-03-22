// dawn/src/io/sysinfo.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sysinfo.hxx"

#include <fixedmath.h>
#include <sys/sysinfo.h>

using namespace dawn;

int CIOSysinfo::init()
{
  if (getCls() == IO_CLASS_SYSTEM_CPULOAD && getDtype() != SObjectId::DTYPE_FLOAT &&
      getDtype() != SObjectId::DTYPE_UB16)
    {
      DAWNERR("CPULOAD requires DTYPE_FLOAT or DTYPE_UB16\n");
      return -EINVAL;
    }

  return OK;
}

size_t CIOSysinfo::getDataDim() const
{
  return dim;
}

CIOSysinfo::~CIOSysinfo()
{
}

size_t CIOSysinfo::getDataDim(uint16_t cls) const
{
  switch (cls)
    {
      case IO_CLASS_SYSTEM_UPTIME:
        {
          return 1;
        }

      case IO_CLASS_SYSTEM_CPULOAD:
        {
          return 3;
        }

      default:
        {
          DAWNERR("Invalid sysinfo class %d\n", cls);
          return 0;
        }
    }
}

int CIOSysinfo::getDataImpl(IODataCmn &data, size_t len)
{
  struct sysinfo sys_info;

  // No batch supported

  if (len != 1)
    {
      return -ENOTSUP;
    }

  /* Get system info */

  sysinfo(&sys_info);

  switch (getCls())
    {
      case IO_CLASS_SYSTEM_UPTIME:
        {
          uint64_t *tmp = static_cast<uint64_t *>(data.getDataPtr());

          *tmp = sys_info.uptime;
          break;
        }

      case IO_CLASS_SYSTEM_CPULOAD:
        {
          switch (getDtype())
            {
              case SObjectId::DTYPE_FLOAT:
                {
                  float *tmp = static_cast<float *>(data.getDataPtr());
                  float f_load = 1.0f / (1 << SI_LOAD_SHIFT);

                  tmp[0] = (float)sys_info.loads[0] * f_load;
                  tmp[1] = (float)sys_info.loads[1] * f_load;
                  tmp[2] = (float)sys_info.loads[2] * f_load;

                  break;
                }

              case SObjectId::DTYPE_UB16:
                {
                  DAWNASSERT(SI_LOAD_SHIFT == 16, "Invalid system configuration");

                  ub16_t *tmp = static_cast<ub16_t *>(data.getDataPtr());

                  tmp[0] = sys_info.loads[0];
                  tmp[1] = sys_info.loads[1];
                  tmp[2] = sys_info.loads[2];

                  break;
                }

              default:
                {
                  DAWNERR("Unsupported data type %d for CPULOAD\n", getDtype());
                  return -EINVAL;
                }
            }
          break;
        }

      default:
        {
          return -EINVAL;
        }
    }

  return OK;
}

int CIOSysinfo::setDataImpl(IODataCmn &data)
{
  UNUSED(data);

  return -ENOTSUP;
}

size_t CIOSysinfo::getDataSize() const
{
  switch (getCls())
    {
      case IO_CLASS_SYSTEM_UPTIME:
        {
          return sizeof(uint64_t) * dim;
        }

      case IO_CLASS_SYSTEM_CPULOAD:
        {
          return sizeof(float) * dim;
        }

      default:
        {
          return 0;
        }
    }
}
