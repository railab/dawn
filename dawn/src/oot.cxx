// dawn/src/oot.cxx
//
// SPDX-License-Identifier: Apache-2.0
//
// Weak default implementations of the OOT user-extension hooks (factories
// and app lifecycle callbacks). Out-of-tree projects override any subset
// with strong definitions in <oot>/external/dawn_oot_hooks.cxx, which
// dawn/apps/dawn/CMakeLists.txt compiles into the application target.

#include <nuttx/compiler.h>
#include <unistd.h>

#include "dawn/oot.hxx"

namespace dawn::oot
{

// Factory hooks.

weak_function IIOFactory *user_io_factory()
{
  return nullptr;
}

weak_function IProgFactory *user_prog_factory()
{
  return nullptr;
}

weak_function IProtoFactory *user_proto_factory()
{
  return nullptr;
}

// Lifecycle hooks.

weak_function int user_init()
{
  return OK;
}

weak_function int user_post_load(CDawn *dawn)
{
  (void)dawn;
  return OK;
}

weak_function void user_on_idle(CDawn *dawn)
{
  (void)dawn;
  sleep(1);
}

weak_function int user_pre_shutdown(CDawn *dawn)
{
  (void)dawn;
  return OK;
}

} // namespace dawn::oot
