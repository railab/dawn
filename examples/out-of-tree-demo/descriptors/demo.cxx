// examples/out-of-tree-demo/descriptors/demo.cxx
//
// SPDX-License-Identifier: Apache-2.0
//
// Hand-written C++ descriptor for the Dawn out-of-tree demo. Exercises the
// user-extension path end-to-end:
//   - one user IO    (CIOMyDummy)
//   - one user PROG  (CProgMyDummy)
//   - one user PROTO (CProtoMyDummy)
//   - the standard pretty-shell PROTO bound to the user IO
//
// The user classes are pulled in via the OOT include directory configured
// in dawn/cmake/dawn_oot.cmake.

#include "dawn/common/descriptor.hxx"

#include "dawn/io/buttons.hxx"
#include "dawn/io/leds.hxx"
#include "dawn/io/uuid.hxx"
#include "dawn/proto/shell/pretty.hxx"

#include "my_io_dummy.hxx"
#include "my_prog_dummy.hxx"
#include "my_proto_dummy.hxx"

using namespace dawn;
using namespace oot_demo;

// Object Definitions

#define MYIODUMMY1     CIOMyDummy::objectId(0)
#define LEDS1          CIOLeds::objectId(false, 0)
#define BUTTONS1       CIOButtons::objectId(false, 0)
#define UUID1          CIOUuid::objectId(0)

#define MYPROGDUMMY1   CProgMyDummy::objectId(0)
#define MYPROTODUMMY1  CProtoMyDummy::objectId(0)
#define SHELL1         CProtoShellPretty::objectId(0)

uint32_t g_dawn_desc[] =
{
  // Header - total object count (including metadata)

  CDescriptor::DAWN_DESCRIPTOR_HDR, 8,

  // Metadata

  CDescriptor::objectId(1), 2,
    CDescriptor::cfgIdVersion(),
      0x00010000,
    CDescriptor::cfgIdString(2),
      0x6f746f6f,    // "ooto" (little-endian "oot-demo")
      0x6d65642d,    // "-dem"

  // myiodummy1 (user IO, with custom init-value cfg item)

  MYIODUMMY1, 1,
    CIOMyDummy::cfgIdInitval(),
      0xdeadbeef,

  // leds1

  LEDS1, 1,
    CIOCommon::cfgIdDevno(),
      0,

  // buttons1

  BUTTONS1, 1,
    CIOCommon::cfgIdDevno(),
      0,

  // uuid1

  UUID1, 0,

  // myprogdummy1 (user PROG, binds to myiodummy1, with custom tag cfg)

  MYPROGDUMMY1, 2,
    CProgMyDummy::cfgIdIOBind(1),
      MYIODUMMY1,
    CProgMyDummy::cfgIdTag(),
      0xcafebabe,

  // myprotodummy1 (user PROTO, binds to myiodummy1)

  MYPROTODUMMY1, 1,
    CProtoMyDummy::cfgIdIOBind(1),
      MYIODUMMY1,

  // shell1 (built-in PROTO, binds to all IOs and the user PROG)

  SHELL1, 1,
    CProtoShellPretty::cfgIdIOBind(5),
      MYIODUMMY1,
      LEDS1,
      BUTTONS1,
      UUID1,
      MYPROGDUMMY1,

  // Footer + checksum placeholder

  CDescriptor::DAWN_DESCRIPTOR_FOOT,
    0xdeadbeef
};

size_t g_dawn_desc_size = sizeof(g_dawn_desc);
