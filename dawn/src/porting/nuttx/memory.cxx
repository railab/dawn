// dawn/src/porting/nuttx/memory.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/memory.hxx"

#include <cstring>
#include <errno.h>
#include <nuttx/mm/mm.h>

namespace dawn
{
namespace porting
{

int memory_pool_init(memory_pool_s *pool, const char *name, void *buffer, size_t size)
{
  if (pool == nullptr || buffer == nullptr || size == 0)
    {
      return -EINVAL;
    }

  if (pool->heap != nullptr)
    {
      return OK;
    }

  pool->heap = mm_initialize(name, buffer, size);
  return pool->heap == nullptr ? -ENOMEM : OK;
}

void *memory_pool_zalloc(memory_pool_s *pool, size_t size)
{
  void *ptr;

  if (pool == nullptr || pool->heap == nullptr || size == 0)
    {
      return nullptr;
    }

  ptr = mm_malloc(static_cast<mm_heap_s *>(pool->heap), size);
  if (ptr != nullptr)
    {
      std::memset(ptr, 0, size);
    }

  return ptr;
}

void memory_pool_free(memory_pool_s *pool, void *ptr)
{
  if (pool == nullptr || pool->heap == nullptr || ptr == nullptr)
    {
      return;
    }

  mm_free(static_cast<mm_heap_s *>(pool->heap), ptr);
}

} // namespace porting
} // namespace dawn
