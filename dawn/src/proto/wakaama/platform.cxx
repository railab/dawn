// dawn/src/proto/wakaama/platform.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "internal.hxx"

#include "dawn/porting/memory.hxx"

#include <cstddef>
#include <cstring>
#include <ctime>
#include <mutex>
#include <strings.h>

using namespace dawn::wakaama_internal;

namespace
{
alignas(std::max_align_t) uint8_t g_pool[CONFIG_DAWN_PROTO_WAKAAMA_MEMORY_POOL_SIZE];
dawn::porting::memory_pool_s g_allocator = {};
std::mutex g_lock;

bool initAllocator()
{
  return dawn::porting::memory_pool_init(&g_allocator, "wakaama", g_pool, sizeof(g_pool)) == OK;
}
} // namespace

extern "C"
{
  void *lwm2m_malloc(size_t size)
  {
    std::lock_guard<std::mutex> lock(g_lock);
    size_t allocSize;
    void *ptr;

    if (!initAllocator())
      {
        return nullptr;
      }

    allocSize = size == 0 ? 1 : size;
    ptr = dawn::porting::memory_pool_zalloc(&g_allocator, allocSize);
    if (ptr == nullptr)
      {
        DAWNERR("Wakaama allocator exhausted: size=%zu pool=%zu\n",
                size,
                static_cast<size_t>(sizeof(g_pool)));
      }

    return ptr;
  }

  void lwm2m_free(void *ptr)
  {
    if (ptr == nullptr)
      {
        return;
      }

    std::lock_guard<std::mutex> lock(g_lock);
    dawn::porting::memory_pool_free(&g_allocator, ptr);
  }

  char *lwm2m_strdup(const char *str)
  {
    char *dup;
    size_t len;

    if (str == nullptr)
      {
        return nullptr;
      }

    len = std::strlen(str) + 1;
    dup = static_cast<char *>(lwm2m_malloc(len));
    if (dup != nullptr)
      {
        std::memcpy(dup, str, len);
      }

    return dup;
  }

  int lwm2m_strncmp(const char *str1, const char *str2, size_t len)
  {
    return std::strncmp(str1, str2, len);
  }

  int lwm2m_strcasecmp(const char *str1, const char *str2)
  {
    return strcasecmp(str1, str2);
  }

  time_t lwm2m_gettime(void)
  {
    return time(nullptr);
  }
}
