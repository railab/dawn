// dawn/include/dawn/porting/memory.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>

namespace dawn
{
namespace porting
{
/**
 * @brief Platform-backed memory pool handle.
 */

struct memory_pool_s
{
  void *heap;
};

/**
 * @brief Initialize a bounded allocator over caller-provided storage.
 */

int memory_pool_init(memory_pool_s *pool, const char *name, void *buffer, size_t size);

/**
 * @brief Allocate zeroed memory from a bounded allocator.
 */

void *memory_pool_zalloc(memory_pool_s *pool, size_t size);

/**
 * @brief Return memory allocated from a bounded allocator.
 */

void memory_pool_free(memory_pool_s *pool, void *ptr);

} // namespace porting
} // namespace dawn
