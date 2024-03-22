// dawn/include/dawn/io/ddata.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <new>

#include "dawn/io/idata.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/** @brief Heap-allocated dynamic I/O data buffer. */

struct io_ddata_t : public io_data_cmn_t
{
  uint8_t *data;   //< Heap-allocated data buffer.
  size_t T;        //< Element size in bytes (1, 2, 4, or 8).
  size_t N;        //< Number of elements per batch.
  size_t M;        //< Number of batches.
  size_t len;      //< Total buffer length in bytes.
  size_t off;      //< Aligned batch offset in bytes.
  uint8_t dtype;   //< Data type enum for introspection.
  bool hasTs;      //< Whether this buffer has per-batch timestamp storage.
  io_ts_t dummyTs; //< Dummy timestamp for API compatibility when hasTs is false.

  /** @brief Align size to word boundary. */

  constexpr size_t ALIGN_TO_WORD_SIZE(size_t size, size_t word_size)
  {
    return (size + word_size - 1) & ~(word_size - 1);
  };

  /**
   * @brief Constructor - allocate heap buffer with M batches.
   *
   * @param t Element size bytes (1, 2, 4, or 8).
   * @param n Elements per batch (default 1).
   * @param m Number of batches (default 1).
   * @param dt Data type enum (default 0).
   * @param ts Whether to include per-batch timestamp (default false).
   */

  explicit io_ddata_t(size_t t, size_t n = 1, size_t m = 1, uint8_t dt = 0, bool ts = false)
    : dummyTs(0)
  {
    size_t tsSize = ts ? sizeof(io_ts_t) : 0;

    assert(t == 1 || t == 2 || t == 4 || t == 8);

    this->T = t;
    this->N = n;
    this->M = m;
    this->dtype = dt;
    this->hasTs = ts;
    this->off = ts ? ALIGN_TO_WORD_SIZE(t * n + tsSize, sizeof(uintptr_t)) : t * n;
    this->len = this->off * m;

    /* Allocate data buffer */

    this->data = new (std::nothrow) uint8_t[this->len]();
  }

  io_ddata_t(const io_ddata_t &) = delete;
  io_ddata_t &operator=(const io_ddata_t &) = delete;

  /** @brief Destructor - deallocate heap buffer. */

  ~io_ddata_t() override
  {
    delete[] data;
  };

  bool isAllocated() const
  {
    return data != nullptr;
  }

  /**
   * @brief Get data element by index and batch (type-safe).
   *
   * @param index Element index within batch.
   * @param batch Batch index (default 0).
   * @return Reference to data element.
   */

  template<typename T>
  constexpr T &get(size_t index, size_t batch = 0)
  {
    size_t tsOff = this->hasTs ? sizeof(io_ts_t) : 0;

    assert(index < this->N);
    assert(batch < this->M);

    size_t i = this->off * batch + index * sizeof(T) + tsOff;
    T *ptr = reinterpret_cast<T *>(&data[i]);

    return *ptr;
  }

  /**
   * @brief Get timestamp reference via bracket operator.
   *
   * @param batch Batch index.
   * @return Reference to timestamp.
   */

  io_ts_t &operator[](size_t batch)
  {
    assert(batch < this->M);
    return this->getTs(batch);
  }

  /** @brief Get element size in bytes. */

  constexpr size_t getSize() const
  {
    return this->T;
  }

  /** @brief Get number of items per batch. */

  constexpr size_t getItems() override
  {
    return this->N;
  }

  /** @brief Get number of batches. */

  constexpr size_t getBatch() const
  {
    return this->M;
  }

  /** @brief Get total buffer size in bytes. */

  constexpr size_t getBufferSize() const
  {
    return this->len;
  }

  /**
   * @brief Get data size in bytes.
   *
   * Returns the size of a single data item (without timestamp if present).
   */

  constexpr size_t getDataSize() override
  {
    return this->T * this->N;
  }

  /**
   * @brief Get pointer to batch.
   *
   * When hasTs is true, returns pointer to [timestamp + data].
   * When hasTs is false, returns pointer to [data] (same as getDataPtr).
   *
   * @param batch Batch index (default 0).
   * @return Void pointer to batch.
   */

  constexpr void *getPtr(size_t batch = 0) override
  {
    return (void *)&data[this->off * batch];
  }

  /**
   * @brief Get pointer to data only (skips timestamp if present).
   *
   * @param batch Batch index (default 0).
   * @return Void pointer to data.
   */

  constexpr void *getDataPtr(size_t batch = 0) override
  {
    size_t tsOff = this->hasTs ? sizeof(io_ts_t) : 0;
    return (void *)&data[this->off * batch + tsOff];
  }

  /** @brief Check if this buffer has per-batch timestamp storage. */

  bool hasTimestamp() override
  {
    return this->hasTs;
  }

  /**
   * @brief Get timestamp reference for batch.
   *
   * When hasTs is true, returns per-batch timestamp from buffer.
   * When hasTs is false, returns shared dummy reference.
   *
   * @param batch Batch index (default 0).
   * @return Reference to timestamp.
   */

  io_ts_t &getTs(size_t batch = 0) override
  {
    if (!this->hasTs)
      {
        return dummyTs;
      }

    size_t i = this->off * batch;
    io_ts_t *ptr = reinterpret_cast<io_ts_t *>(&this->data[i]);
    return *ptr;
  };

  /** @brief Get data type enum for introspection. */

  constexpr uint8_t getDtype() const
  {
    return this->dtype;
  }
};

} // Namespace dawn
