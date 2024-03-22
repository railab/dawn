// dawn/include/dawn/io/viewdata.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>

#include "dawn/io/idata.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/** @brief Non-owning I/O data view over caller-provided storage. */

struct io_data_view_t : public io_data_cmn_t
{
  void *buffer;    ///< Caller-owned data buffer.
  size_t bytes;    ///< Buffer size in bytes.
  size_t items;    ///< Number of data items in the buffer.
  io_ts_t dummyTs; ///< Dummy timestamp for API compatibility.

  /**
   * @brief Constructor.
   *
   * @param dataBuffer Caller-owned data buffer.
   * @param dataBytes Buffer size in bytes.
   * @param dataItems Number of data items in the buffer (default 1).
   */

  io_data_view_t(void *dataBuffer, size_t dataBytes, size_t dataItems = 1)
    : buffer(dataBuffer)
    , bytes(dataBytes)
    , items(dataItems)
    , dummyTs(0)
  {
  }

  /** @brief Get number of items in the viewed buffer. */

  size_t getItems() override
  {
    return items;
  }

  /** @brief Get viewed data size in bytes. */

  size_t getDataSize() override
  {
    return bytes;
  }

  /**
   * @brief Get pointer to viewed buffer.
   *
   * @param batch Batch index (ignored; views expose one contiguous buffer).
   * @return Void pointer to viewed buffer.
   */

  void *getPtr(size_t batch = 0) override
  {
    (void)batch;
    return buffer;
  }

  /**
   * @brief Get pointer to viewed data.
   *
   * @param batch Batch index (ignored; views expose one contiguous buffer).
   * @return Void pointer to viewed data.
   */

  void *getDataPtr(size_t batch = 0) override
  {
    (void)batch;
    return buffer;
  }

  /** @brief This view has no per-batch timestamp storage. */

  bool hasTimestamp() override
  {
    return false;
  }

  /**
   * @brief Get timestamp reference (returns shared dummy).
   *
   * @param batch Batch index (ignored).
   * @return Reference to dummy timestamp.
   */

  io_ts_t &getTs(size_t batch = 0) override
  {
    (void)batch;
    return dummyTs;
  }
};

} // Namespace dawn
