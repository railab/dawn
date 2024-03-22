// dawn/src/prog/common.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/common.hxx"

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#ifdef CONFIG_DAWN_IO_VIRT
#  include "dawn/io/virt.hxx"
#endif

using namespace dawn;

CProgCommon::CProgCommon(CDescObject &desc)
  : CBindableObject(desc)
{
  DAWNASSERT(desc.getObjectId().s.cls > PROG_CLASS_ANY &&
               desc.getObjectId().s.cls < PROG_CLASS_LAST,
             "invalid class");
}

int CProgCommon::prepareWritableTarget(CIOCommon *io, size_t dim, bool notify)
{
  if (io == nullptr)
    {
      return -EINVAL;
    }

  if (!io->isWrite())
    {
      DAWNERR("target 0x%" PRIx32 " is not writable\n", io->getIdV());
      return -EINVAL;
    }

  if (io->getCls() != CIOCommon::IO_CLASS_VIRT)
    {
      return OK;
    }

#ifdef CONFIG_DAWN_IO_VIRT
  if (io->getDataDim() == 0)
    {
      return reinterpret_cast<CIOVirt *>(io)->initialize(dim, 1, notify);
    }

  if (io->getDataDim() != dim)
    {
      DAWNERR("virt target 0x%" PRIx32 " dimension mismatch: expected %zu got %zu\n",
              io->getIdV(),
              dim,
              io->getDataDim());
      return -EINVAL;
    }

  if (io->isNotify() != notify)
    {
      DAWNERR("virt target 0x%" PRIx32 " notify mismatch: expected %d got %d\n",
              io->getIdV(),
              notify,
              io->isNotify());
      return -EINVAL;
    }

  return OK;
#else
  UNUSED(dim);
  UNUSED(notify);
  DAWNERR("virtIO target 0x%" PRIx32 " requires CONFIG_DAWN_IO_VIRT\n", io->getIdV());
  return -ENOTSUP;
#endif
}
