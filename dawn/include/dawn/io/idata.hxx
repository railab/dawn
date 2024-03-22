// dawn/include/dawn/io/idata.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>

#include "dawn/porting/config.hxx"

namespace dawn
{
/** @brief Timestamp data type (uint64_t, typically microseconds since boot). */

typedef uint64_t io_ts_t;

/** @brief Base interface for I/O data buffers (static and dynamic). */

struct io_data_cmn_t
{
  /** @brief Get number of items per batch. */

  virtual size_t getItems() = 0;

  /**
   * @brief Get data size in bytes.
   *
   * Returns the size of a single data item (without timestamp if present).
   */

  virtual size_t getDataSize() = 0;

  /**
   * @brief Get pointer to batch.
   *
   * When timestamp is enabled, returns pointer to [timestamp + data].
   * When timestamp is disabled, returns pointer to [data] (same as
   * getDataPtr).
   *
   * @param batch Batch index (default 0).
   * @return Void pointer to batch.
   */

  virtual void *getPtr(size_t batch = 0) = 0;

  /**
   * @brief Get pointer to data only (skips timestamp if present).
   *
   * @param batch Batch index (default 0).
   * @return Void pointer to data.
   */

  virtual void *getDataPtr(size_t batch = 0) = 0;

  /**
   * @brief Check if this data buffer has per-batch timestamp storage.
   *
   * @return True if timestamps are stored per-batch.
   */

  virtual bool hasTimestamp() = 0;

  /**
   * @brief Get timestamp reference for batch.
   *
   * When hasTimestamp() is true, returns a per-batch timestamp reference.
   * When hasTimestamp() is false, returns a shared dummy reference
   * (writes are silently discarded on next call).
   *
   * @param batch Batch index (default 0).
   * @return Reference to timestamp.
   */

  virtual uint64_t &getTs(size_t batch = 0) = 0;

  /** @brief Virtual destructor for polymorphic cleanup. */

  virtual ~io_data_cmn_t() {};
} typedef IODataCmn;

} // Namespace dawn
