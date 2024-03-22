// dawn/src/dev/descriptor.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/dev/descriptor.hxx"

#include <cerrno>
#include <cstring>

using namespace dawn;

CDevDescriptor *CDevDescriptor::singleton = nullptr;

int CDevDescriptor::regDescriptor(int inst, const CDevDescriptor::SDescriptorReg &reg)
{
  if (inst < 0 || inst >= MAX_DESCRIPTORS)
    {
      return -ENOMEM;
    }

  if (inst > 0 && (reg.ptr == nullptr || reg.len == 0))
    {
      return resetSlot(inst);
    }

  regDesc[inst].ptr = reg.ptr;
  regDesc[inst].len = reg.len;
#if CONFIG_DAWN_DESC_SLOTS > 1
  if (inst > 0)
    {
      slotWritten[static_cast<size_t>(inst - 1)] = reg.len;
    }
#endif

  return OK;
}

int CDevDescriptor::getDescriptor(int inst, CDevDescriptor::SDescriptorReg &reg)
{
  if (inst < 0 || inst >= MAX_DESCRIPTORS)
    {
      return -ENOMEM;
    }

  reg.ptr = regDesc[inst].ptr;
  reg.len = regDesc[inst].len;

  return OK;
}

int CDevDescriptor::writeSlotData(int inst, const void *data, size_t offset, size_t len)
{
#if CONFIG_DAWN_DESC_SLOTS <= 1
  (void)inst;
  (void)data;
  (void)offset;
  (void)len;
  return -ENOTSUP;
#else
  size_t slot_idx;
  size_t end;

  if (inst <= 0 || inst >= MAX_DESCRIPTORS)
    {
      return -EINVAL;
    }

  if (len > 0 && data == nullptr)
    {
      return -EINVAL;
    }

  if (offset > SLOT_SIZE || len > SLOT_SIZE - offset)
    {
      return -ENOSPC;
    }

  slot_idx = static_cast<size_t>(inst - 1);
  end = offset + len;

  if (len > 0)
    {
      std::memcpy(&slotBuf[slot_idx][offset], data, len);
    }

  if (offset == 0)
    {
      slotWritten[slot_idx] = len;
    }
  else if (end > slotWritten[slot_idx])
    {
      slotWritten[slot_idx] = end;
    }

  regDesc[inst].ptr = slotBuf[slot_idx].data();
  regDesc[inst].len = slotWritten[slot_idx];

  return OK;
#endif
}

size_t CDevDescriptor::getSlotWritten(int inst) const
{
#if CONFIG_DAWN_DESC_SLOTS <= 1
  (void)inst;
  return 0;
#else
  if (inst <= 0 || inst >= MAX_DESCRIPTORS)
    {
      return 0;
    }

  return slotWritten[static_cast<size_t>(inst - 1)];
#endif
}

int CDevDescriptor::resetSlot(int inst)
{
#if CONFIG_DAWN_DESC_SLOTS <= 1
  (void)inst;
  return -ENOTSUP;
#else
  size_t slot_idx;

  if (inst <= 0 || inst >= MAX_DESCRIPTORS)
    {
      return -EINVAL;
    }

  slot_idx = static_cast<size_t>(inst - 1);
  std::memset(slotBuf[slot_idx].data(), 0, SLOT_SIZE);
  slotWritten[slot_idx] = 0;
  regDesc[inst].ptr = nullptr;
  regDesc[inst].len = 0;

  return OK;
#endif
}
