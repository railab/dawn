// dawn/src/proto/common.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/common.hxx"

#include "dawn/debug.hxx"

using namespace dawn;

CProtoCommon::CProtoCommon(CDescObject &desc)
  : CBindableObject(desc)
{
  DAWNASSERT(desc.getObjectId().s.cls > PROTO_CLASS_ANY &&
               desc.getObjectId().s.cls < PROTO_CLASS_LAST,
             "invalid class");
}
