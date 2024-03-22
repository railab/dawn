// dawn/src/common/objectid.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/objectid.hxx"

#include "dawn/debug.hxx"

using namespace dawn;

int SObjectId::getDtypeSize_(const EObjectDataType dtype)
{
  switch (dtype)
    {
#if defined(CONFIG_DAWN_DTYPE_BOOL) || defined(CONFIG_DAWN_DTYPE_INT8) || \
  defined(CONFIG_DAWN_DTYPE_UINT8) || defined(CONFIG_DAWN_DTYPE_BLOCK)
#  ifdef CONFIG_DAWN_DTYPE_BOOL
      case DTYPE_BOOL:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT8
      case DTYPE_INT8:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT8
      case DTYPE_UINT8:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_BLOCK
      case DTYPE_BLOCK:
#  endif
        {
          return 1;
        }
#endif

#if defined(CONFIG_DAWN_DTYPE_INT16) || defined(CONFIG_DAWN_DTYPE_UINT16)
#  ifdef CONFIG_DAWN_DTYPE_INT16
      case DTYPE_INT16:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT16
      case DTYPE_UINT16:
#  endif
        {
          return 2;
        }
#endif

#if defined(CONFIG_DAWN_DTYPE_INT32) || defined(CONFIG_DAWN_DTYPE_UINT32) || \
  defined(CONFIG_DAWN_DTYPE_FLOAT) || defined(CONFIG_DAWN_DTYPE_B16) ||      \
  defined(CONFIG_DAWN_DTYPE_UB16)
#  ifdef CONFIG_DAWN_DTYPE_INT32
      case DTYPE_INT32:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT32
      case DTYPE_UINT32:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_FLOAT
      case DTYPE_FLOAT:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_B16
      case DTYPE_B16:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UB16
      case DTYPE_UB16:
#  endif
        {
          return 4;
        }
#endif

#if defined(CONFIG_DAWN_DTYPE_INT64) || defined(CONFIG_DAWN_DTYPE_DOUBLE) || \
  defined(CONFIG_DAWN_DTYPE_UINT64)
#  ifdef CONFIG_DAWN_DTYPE_INT64
      case DTYPE_INT64:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_DOUBLE
      case DTYPE_DOUBLE:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT64
      case DTYPE_UINT64:
#  endif
        {
          return 8;
        }
#endif

      default:
        {
          return -1;
        }
    }

  return 0;
}

bool SObjectId::isDtypeSupported(uint8_t dtype)
{
  switch (dtype)
    {
      case DTYPE_ANY:
        return true;

#ifdef CONFIG_DAWN_DTYPE_BOOL
      case DTYPE_BOOL:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT8
      case DTYPE_INT8:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
      case DTYPE_UINT8:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
      case DTYPE_INT16:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
      case DTYPE_UINT16:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
      case DTYPE_INT32:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
      case DTYPE_UINT32:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT64
      case DTYPE_INT64:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT64
      case DTYPE_UINT64:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case DTYPE_FLOAT:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
      case DTYPE_DOUBLE:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_B16
      case DTYPE_B16:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UB16
      case DTYPE_UB16:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_CHAR
      case DTYPE_CHAR:
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_BLOCK
      case DTYPE_BLOCK:
        return true;
#endif
      default:
        return false;
    }
}

const char *SObjectId::dtypeToString(uint8_t dtype)
{
  switch (dtype)
    {
      case DTYPE_ANY:
        return "any";
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case DTYPE_BOOL:
        return "bool";
#endif
#ifdef CONFIG_DAWN_DTYPE_INT8
      case DTYPE_INT8:
        return "int8";
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
      case DTYPE_UINT8:
        return "uint8";
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
      case DTYPE_INT16:
        return "int16";
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
      case DTYPE_UINT16:
        return "uint16";
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
      case DTYPE_INT32:
        return "int32";
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
      case DTYPE_UINT32:
        return "uint32";
#endif
#ifdef CONFIG_DAWN_DTYPE_INT64
      case DTYPE_INT64:
        return "int64";
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT64
      case DTYPE_UINT64:
        return "uint64";
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case DTYPE_FLOAT:
        return "float";
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
      case DTYPE_DOUBLE:
        return "double";
#endif
#ifdef CONFIG_DAWN_DTYPE_B16
      case DTYPE_B16:
        return "b16";
#endif
#ifdef CONFIG_DAWN_DTYPE_UB16
      case DTYPE_UB16:
        return "ub16";
#endif
#ifdef CONFIG_DAWN_DTYPE_CHAR
      case DTYPE_CHAR:
        return "char";
#endif
#ifdef CONFIG_DAWN_DTYPE_BLOCK
      case DTYPE_BLOCK:
        return "block";
#endif
      default:
        return "???";
    }
}
