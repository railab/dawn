// dawn/include/dawn/io/sdata.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include "dawn/io/idata.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/** @brief Single batch of I/O data with timestamp. */

template<typename T, size_t N>
struct io_data_s
{
  io_ts_t ts; ///< Timestamp for this measurement
  T data[N];  ///< Data elements (N items of type T)
};

/** @brief Single batch of I/O data without timestamp. */

template<typename T, size_t N>
struct io_data_nots_s
{
  T data[N]; ///< Data elements (N items of type T)
};

/** @brief Static (compile-time) I/O data buffer (no timestamp). */

template<typename T, size_t N, size_t M = 1, bool TS = false>
struct io_sdata_t : public io_data_cmn_t
{
  io_data_nots_s<T, N> data[M]; ///< Array of M batches (each: N elements, no timestamp)
  io_ts_t dummyTs;              ///< Dummy timestamp for API compatibility

  /** @brief Constructor - initialize all data to zero. */

  io_sdata_t()
    : data{}
    , dummyTs(0)
  {
    assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
  }

  /**
   * @brief Get data element by index and batch.
   *
   * @param index Element index within batch.
   * @param batch Batch index (default 0).
   * @return Reference to data element.
   */

  constexpr T &operator()(size_t index, size_t batch = 0)
  {
    assert(index < N);
    assert(batch < M);
    return data[batch].data[index];
  }

  /**
   * @brief Get timestamp (returns shared dummy).
   *
   * @param batch Batch index (ignored).
   * @return Reference to dummy timestamp.
   */

  constexpr io_ts_t &operator[](size_t batch)
  {
    (void)batch;
    return dummyTs;
  }

  /** @brief Get element size in bytes. */

  constexpr static size_t getSize()
  {
    return sizeof(T);
  }

  /** @brief Get number of items per batch. */

  constexpr size_t getItems() override
  {
    return N;
  }

  /** @brief Get number of batches. */

  constexpr static size_t getBatch()
  {
    return M;
  }

  /** @brief Get total buffer size in bytes. */

  constexpr size_t getBufferSize() const
  {
    return sizeof(this->data);
  }

  /**
   * @brief Get data size in bytes.
   *
   * Returns the size of a single data item (without timestamp).
   */

  constexpr size_t getDataSize() override
  {
    return sizeof(T) * N;
  }

  /**
   * @brief Get pointer to batch (data only, no timestamp).
   *
   * @param batch Batch index (default 0).
   * @return Void pointer to batch.
   */

  constexpr void *getPtr(size_t batch = 0) override
  {
    return (void *)&data[batch];
  }

  /**
   * @brief Get pointer to data only (same as getPtr for no-timestamp).
   *
   * @param batch Batch index (default 0).
   * @return Void pointer to data.
   */

  constexpr void *getDataPtr(size_t batch = 0) override
  {
    return (void *)&data[batch];
  }

  /** @brief This buffer has no per-batch timestamp. */

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

  constexpr io_ts_t &getTs(size_t batch = 0) override
  {
    (void)batch;
    return dummyTs;
  };
};

/** @brief Static (compile-time) I/O data buffer (with timestamp). */

template<typename T, size_t N, size_t M>
struct io_sdata_t<T, N, M, true> : public io_data_cmn_t
{
  /** @brief Array of M batches (each: timestamp + N elements). */

  io_data_s<T, N> data[M];

  /** @brief Constructor - initialize all data to zero. */

  io_sdata_t()
    : data{}
  {
    assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
  }

  /**
   * @brief Get data element by index and batch.
   *
   * @param index Element index within batch.
   * @param batch Batch index (default 0).
   * @return Reference to data element.
   */

  constexpr T &operator()(size_t index, size_t batch = 0)
  {
    assert(index < N);
    assert(batch < M);
    return data[batch].data[index];
  }

  /**
   * @brief Get timestamp for batch.
   *
   * @param batch Batch index.
   * @return Reference to timestamp.
   */

  constexpr io_ts_t &operator[](size_t batch)
  {
    assert(batch < M);
    return data[batch].ts;
  }

  /** @brief Get element size in bytes. */

  constexpr static size_t getSize()
  {
    return sizeof(T);
  }

  /** @brief Get number of items per batch. */

  constexpr size_t getItems() override
  {
    return N;
  }

  /** @brief Get number of batches. */

  constexpr static size_t getBatch()
  {
    return M;
  }

  /** @brief Get total buffer size in bytes. */

  constexpr size_t getBufferSize() const
  {
    return sizeof(this->data);
  }

  /**
   * @brief Get data size in bytes.
   *
   * Returns the size of a single data item (without timestamp if present).
   */

  constexpr size_t getDataSize() override
  {
    return sizeof(T) * N;
  }

  /**
   * @brief Get pointer to batch (timestamp + data).
   *
   * @param batch Batch index (default 0).
   * @return Void pointer to batch.
   */

  constexpr void *getPtr(size_t batch = 0) override
  {
    return (void *)&data[batch];
  }

  /**
   * @brief Get pointer to data only (skips timestamp).
   *
   * @param batch Batch index (default 0).
   * @return Void pointer to data.
   */

  constexpr void *getDataPtr(size_t batch = 0) override
  {
    return (void *)data[batch].data;
  }

  /** @brief This buffer has per-batch timestamp. */

  bool hasTimestamp() override
  {
    return true;
  }

  /**
   * @brief Get timestamp reference for batch.
   *
   * @param batch Batch index (default 0).
   * @return Reference to timestamp.
   */

  constexpr io_ts_t &getTs(size_t batch = 0) override
  {
    return data[batch].ts;
  };
};

} // Namespace dawn
