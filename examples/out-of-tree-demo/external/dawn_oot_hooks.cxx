// examples/out-of-tree-demo/external/dawn_oot_hooks.cxx
//
// SPDX-License-Identifier: Apache-2.0
//
// OOT user-extension hooks. dawn/apps/dawn/CMakeLists.txt compiles this
// file into the application target so the strong definitions below beat
// the weak defaults provided by dawn/src/oot.cxx. The factory class
// implementations live under external/src/{io,prog,proto}/ and are
// compiled into the dawn library.

#include "dawn/debug.hxx"
#include "dawn/oot.hxx"

#include "my_io_factory.hxx"
#include "my_prog_factory.hxx"
#include "my_proto_factory.hxx"

namespace dawn::oot
{

// Factory overrides.

IIOFactory *user_io_factory(void)
{
  static oot_demo::CIOMyFactory s_inst;
  return &s_inst;
}

IProgFactory *user_prog_factory(void)
{
  static oot_demo::CProgMyFactory s_inst;
  return &s_inst;
}

IProtoFactory *user_proto_factory(void)
{
  static oot_demo::CProtoMyFactory s_inst;
  return &s_inst;
}

// Lifecycle hook override (demo): just announce that the OOT hook ran so
// the example exercises the new surface and acts as live documentation.

int user_init(void)
{
  DAWNINFO("oot demo: user_init\n");
  return OK;
}

} // namespace dawn::oot
