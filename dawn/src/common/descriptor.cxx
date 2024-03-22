// dawn/src/common/descriptor.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/descriptor.hxx"

#include <cstring>

#include "dawn/debug.hxx"
#include "dawn/porting/crc.hxx"

using namespace dawn;

#if defined(CONFIG_DAWN_DESC_SLOTS) && CONFIG_DAWN_DESC_SLOTS > 1
#  define DAWN_DESC_CRC_REQUIRED 1
#elif defined(CONFIG_DAWN_DESC_VALID_CRC32)
#  define DAWN_DESC_CRC_REQUIRED 1
#else
#  define DAWN_DESC_CRC_REQUIRED 0
#endif

static int binValidateDtypes(const uint32_t *bin, size_t len)
{
  size_t i;
  size_t words;
  size_t count;

  if (bin == nullptr || len < 8 || len % sizeof(uint32_t))
    {
      return -EINVAL;
    }

  words = len / sizeof(uint32_t);
  i = 2;
  count = bin[1] & 0xffff;

  for (size_t obj = 0; obj < count; obj++)
    {
      SObjectId::ObjectId objid;
      size_t objsize;
      uint8_t dtype;

      if ((i + 2) > words)
        {
          DAWNERR("(i + 2) > words\n");
          return -EINVAL;
        }

      objid = bin[i++];
      dtype = SObjectId::objectIdGetDtype(objid);
      if (!SObjectId::isDtypeSupported(dtype))
        {
          DAWNERR("unsupported object dtype %u (%s)\n", dtype, SObjectId::dtypeToString(dtype));
          return -ENOTSUP;
        }

      objsize = bin[i++];

      for (size_t cfg = 0; cfg < objsize; cfg++)
        {
          SObjectCfg::ObjectCfgId cfgid;
          size_t cfgsize;

          if (i >= words)
            {
              DAWNERR("i >= words\n");
              return -EINVAL;
            }

          cfgid = bin[i++];
          dtype = SObjectCfg::objectCfgGetDtype(cfgid);
          if (!SObjectId::isDtypeSupported(dtype))
            {
              DAWNERR("unsupported config dtype %u (%s)\n", dtype, SObjectId::dtypeToString(dtype));
              return -ENOTSUP;
            }

          cfgsize = SObjectCfg::objectCfgGetSize(cfgid);
          if ((i + cfgsize) > words)
            {
              DAWNERR("(i + cfgsize) > words\n");
              return -EINVAL;
            }

          i += cfgsize;
        }
    }

  if (i != words)
    {
      DAWNERR("i != words\n");
      return -EINVAL;
    }

  return OK;
}

void CDescriptor::binDump(const uint32_t *bin, size_t len)
{
  SObjectCfg::UObjectCfgId objcfg;
  SObjectId::ObjectId tmp = 0;
  size_t i = 0;

  // Ignore warnings
  UNUSED(tmp);

  DAWNINFO("\n");
  DAWNINFO("\n");
  DAWNINFO("BINDUMP ptr:%p len:%zu:\n\n", bin, len);
  tmp = bin[i++];
  UNUSED(tmp);
  DAWNINFO("\tMAGIC: 0x%" PRIx32 "\n", tmp);
  tmp = bin[i++];
  UNUSED(tmp);
  DAWNINFO("\tObjects: 0x%" PRIx32 "\n", tmp);

  // Ignore check sum for now

  len -= (2 * sizeof(SObjectId::ObjectId));

  while (i < len / 4)
    {
      size_t objsize;

      DAWNINFO("\n");
      tmp = bin[i++];
      UNUSED(tmp);
      DAWNINFO("\tObjectID: 0x%" PRIx32 "\n", tmp);
      objsize = bin[i++];
      DAWNINFO("\t  size: 0x%zx\n", objsize);

      // Print config items

      for (size_t j = 0; j < objsize; j++)
        {
          objcfg.v = bin[i++];
          DAWNINFO("\t  objcfg: 0x%" PRIx32 "\n", objcfg.v);

          for (size_t k = 0; k < objcfg.s.size; k++)
            {
              tmp = bin[i++];
              UNUSED(tmp);
              DAWNINFO("\t    data: 0x%" PRIx32 "\n", tmp);
            }
        }
    }

  tmp = bin[i++];
  UNUSED(tmp);
  DAWNINFO("\tLAST: 0x%" PRIx32 "\n", tmp);
  tmp = bin[i++];
  UNUSED(tmp);
  DAWNINFO("\tCHECK: 0x%" PRIx32 "\n", tmp);

  DAWNINFO("\n");
  DAWNINFO("\n");
}

int CDescriptor::binValid(const uint32_t *bin, size_t len)
{
  size_t i = 0;
  int dret;

  // No magic

  if (bin[i++] != DAWN_DESCRIPTOR_HDR)
    {
      return -EINVAL;
    }

  // No objects - no valid config

  if (bin[i++] == 0)
    {
      return -EINVAL;
    }

#if DAWN_DESC_CRC_REQUIRED
  // Validate check sum
  if (CDescriptor::binCheckValid(bin, len) == false)
    {
      return -EINVAL;
    }
#endif

  // If sum is valid - remove 8 bytes from lenght

  len -= 2 * sizeof(SObjectId::ObjectId);

  dret = binValidateDtypes(bin, len);
  if (dret != OK)
    {
      return dret;
    }

  // Validate descriptor

  return CObject::validateDesc(&bin[i], len - i * sizeof(SObjectId::ObjectId));
}

bool CDescriptor::binCheckValid(const uint32_t *bin, size_t len)
{
  SObjectId::ObjectId sum = crc32(reinterpret_cast<const uint8_t *>(bin), len);

  return (sum == 0);
}

int CDescriptor::binCheckFill(uint32_t *bin, size_t len)
{
  // Check LAST marker

  if (bin[(len / 4) - 2] != CDescriptor::DAWN_DESCRIPTOR_FOOT)
    {
      return -EINVAL;
    }

  bin[(len / 4) - 1] = crc32(reinterpret_cast<const uint8_t *>(bin), len - 4);

  return 0;
}

CDescriptor::~CDescriptor()
{
  reset();
}

void CDescriptor::parseMeta()
{
  size_t words = objectWords();
  uint32_t word;

  if (!bindesc || bindesc->hdr.size == 0)
    {
      return;
    }

  if (words < 2)
    {
      return;
    }

  if (!objectWordAt(0, &word) || SObjectId::objectIdGetType(word) != SObjectId::OBJTYPE_ANY)
    {
      return;
    }

  if (!objectWordAt(1, &word))
    {
      return;
    }

  size_t numItems = word;
  size_t off = 2; // Start after objectId + numItems

  for (size_t i = 0; i < numItems; i++)
    {
      uint32_t cfgid;

      if (off >= words)
        {
          return;
        }

      if (!objectWordAt(off, &cfgid))
        {
          return;
        }

      uint32_t size = (cfgid >> 5) & 0x3FF;
      uint32_t id = cfgid & 0x1F;

      ++off; // Point to first data word
      if (off + size > words)
        {
          return;
        }

      switch (id)
        {
          case DESC_CFG_NO_IDLE_QUIT:
            if (size > 0)
              {
                if (!objectWordAt(off, &word))
                  {
                    return;
                  }
                noIdleQuit = (word != 0);
              }
            break;

          case DESC_CFG_VERSION:
          case DESC_CFG_STRING:
          default:
            // not yet implemented.
            break;
        }

      off += size;
    }
}

void CDescriptor::reset()
{
  for (CDescObject *obj : vobjcfg)
    {
      DAWNINFO("delete desc for 0x%" PRIx32 "\n", obj->getObjectIdV());
      delete obj;
    }

  vobjcfg.clear();
  bindesc = nullptr;
  bindescLen = 0;
  noIdleQuit = false;
}

int CDescriptor::loadBin(uint32_t *bin, size_t len, bool force_valid, bool dump)
{
  reset();

  // Dump config

  if (dump)
    {
      binDump(bin, len);
    }

  if (force_valid)
    {
      // Make sure that footer is reserved

      if (bin[(len / 4) - 2] != CDescriptor::DAWN_DESCRIPTOR_FOOT)
        {
          return -EINVAL;
        }

      // Fill with checksum

      binCheckFill(bin, len);
    }

  // Validate input data

  if (binValid(bin, len) != OK)
    {
      return -EINVAL;
    }

  bindesc = reinterpret_cast<SDescriptorBin *>(bin);
  bindescLen = len;

  // Parse metadata config items.

  parseMeta();

  return OK;
}

CDescriptor::SDescriptorBin *CDescriptor::getBin()
{
  return bindesc;
}

size_t CDescriptor::getBinLen()
{
  return bindescLen;
}

size_t CDescriptor::objectWords() const
{
  if (bindesc == nullptr || bindescLen < sizeof(SDescriptorBinHdr) + sizeof(SDescriptorBinFtr))
    {
      return 0;
    }

  return (bindescLen - sizeof(SDescriptorBinHdr) - sizeof(SDescriptorBinFtr)) / sizeof(uint32_t);
}

bool CDescriptor::objectWordAt(size_t offset, uint32_t *value) const
{
  if (value == nullptr || bindesc == nullptr || offset >= objectWords())
    {
      return false;
    }

  const uint8_t *objects = reinterpret_cast<const uint8_t *>(bindesc) + sizeof(SDescriptorBinHdr);
  std::memcpy(value, objects + offset * sizeof(uint32_t), sizeof(*value));
  return true;
}

SObjectCfg::SObjectCfgData *CDescriptor::objectCfgAtOffset(size_t offset)
{
  if (bindesc == nullptr || offset + 2 > objectWords())
    {
      return nullptr;
    }

  return reinterpret_cast<SObjectCfg::SObjectCfgData *>(
    reinterpret_cast<uint32_t *>(&bindesc->objects) + offset);
};

SObjectCfg::UObjectCfgId *CDescriptor::objectCfgIdAtOffset(size_t offset)
{
  if (bindesc == nullptr || offset >= objectWords())
    {
      return nullptr;
    }

  return reinterpret_cast<SObjectCfg::UObjectCfgId *>(
    reinterpret_cast<uint32_t *>(&bindesc->objects) + offset);
};

void CDescriptor::alloc_objects(CHandler &h, const allocobj_func_t &func)
{
  CDescriptor::SDescriptorBin *cfg = bindesc;
  size_t offset = 0;

  DAWNASSERT(cfg != nullptr, "invalid input");

  // Start from object 0

  for (size_t i = 0; i < cfg->hdr.size; i++)
    {
      SObjectCfg::SObjectCfgData *ocfg;

      // Get object configuration

      ocfg = objectCfgAtOffset(offset);
      if (ocfg == nullptr)
        {
          return;
        }

      // Check if object is for this handler

      if (h.isObjectValid(ocfg->objid))
        {
          // Check for duplicates

          for (const CDescObject *x : vobjcfg)
            {
              SObjectId::ObjectId tmp1 = x->getObjectIdV();
              SObjectId::ObjectId tmp2 = ocfg->objid.v;

              if (tmp1 == tmp2)
                {
                  DAWNERR("duplicate 0x%" PRIx32 " 0x%" PRIx32 " \n", tmp1, tmp2);
                }
            }

          // New configuration object

          CDescObject *desc = new CDescObject(*ocfg);
          vobjcfg.push_back(desc);

          // Call implementation

          func(h, *desc);
        }

      // Objid + size

      offset += (sizeof(SObjectId::UObjectId) + 4) / 4;

      // Go to next object

      for (size_t j = 0; j < ocfg->size; j++)
        {
          uint32_t objcfg;
          if (!objectWordAt(offset, &objcfg))
            {
              return;
            }

          // ObjectCfgId + cfg items

          offset += 1 + SObjectCfg::objectCfgGetSize(objcfg);
        }
    }
}
