// dawn/src/prog/bitwise.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>
#include <cstdint>

#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

namespace dawn
{
namespace prog
{
inline bool isBitwiseDtype(uint8_t dtype)
{
  return dtype != SObjectId::DTYPE_ANY && dtype != SObjectId::DTYPE_CHAR &&
         dtype != SObjectId::DTYPE_BLOCK;
}

inline size_t getLogicalElementBits(uint8_t dtype, size_t storageSize)
{
  return dtype == SObjectId::DTYPE_BOOL ? 1 : storageSize * 8;
}

inline size_t getLogicalBits(uint8_t dtype, size_t storageSize, size_t dim)
{
  return dim * getLogicalElementBits(dtype, storageSize);
}

inline size_t getLogicalBits(const CIOCommon *io)
{
  return getLogicalBits(io->getDtype(), io->getDtypeSize(), io->getDataDim());
}

inline size_t getLogicalBits(io_ddata_t *data)
{
  return getLogicalBits(data->getDtype(), data->getSize(), data->getItems());
}

inline bool readLogicalBit(uint8_t dtype, size_t storageSize, const void *data, size_t bitIndex)
{
  const size_t elementBits = getLogicalElementBits(dtype, storageSize);
  const size_t element = bitIndex / elementBits;
  const size_t bitInElem = bitIndex % elementBits;
  const uint8_t *ptr = static_cast<const uint8_t *>(data) + element * storageSize;

  if (dtype == SObjectId::DTYPE_BOOL)
    {
      return ptr[0] != 0;
    }

  return ((ptr[bitInElem / 8] >> (bitInElem % 8)) & 1u) != 0;
}

inline bool readLogicalBit(const CIOCommon *io, const void *data, size_t bitIndex)
{
  return readLogicalBit(io->getDtype(), io->getDtypeSize(), data, bitIndex);
}

inline void writeLogicalBit(uint8_t dtype,
                            size_t storageSize,
                            void *data,
                            size_t bitIndex,
                            bool value)
{
  const size_t elementBits = getLogicalElementBits(dtype, storageSize);
  const size_t element = bitIndex / elementBits;
  const size_t bitInElem = bitIndex % elementBits;
  uint8_t *ptr = static_cast<uint8_t *>(data) + element * storageSize;

  if (dtype == SObjectId::DTYPE_BOOL)
    {
      ptr[0] = value ? 1 : 0;
      return;
    }

  if (value)
    {
      ptr[bitInElem / 8] |= static_cast<uint8_t>(1u << (bitInElem % 8));
    }
}

inline void writeLogicalBit(CIOCommon *io, void *data, size_t bitIndex, bool value)
{
  writeLogicalBit(io->getDtype(), io->getDtypeSize(), data, bitIndex, value);
}
} // namespace prog
} // namespace dawn
