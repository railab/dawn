// dawn/src/common/object.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/object.hxx"

#include <climits>
#include <cstdio>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/prog/common.hxx"
#include "dawn/proto/common.hxx"

using namespace dawn;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME

// Returns human-readable object name in format "classname_instance".
// Lazily initializes name on first call using virtual getClassNameStr().

const char *CObject::getName() const
{
  // Lazy initialization: generate name on first call
  if (name[0] == '\0')
    {
      const char *class_name = getClassNameStr();
      snprintf(name, sizeof(name), "%s_%u", class_name, uobjid.s.priv);
      name[sizeof(name) - 1] = '\0'; // Ensure null termination
    }
  return name;
}

#endif // CONFIG_DAWN_OBJECT_HAS_NAME

// Default validation method that only checks descriptor lengths.
//
// A negative return code returned by this function identifies the validation
// stage that failed.

int CObject::descValidDefault(const uint32_t *data, size_t len)
{
  SObjectCfg::UObjectCfgId objcfg;
  size_t objsize;
  size_t i = 0;

  // Invalid len

  if (len < 4 || len % sizeof(uint32_t))
    {
      return DESCVALID_ERR_LEN_ALIGN;
    }

  // Normalize len and start len from zero

  len = len / sizeof(uint32_t);
  len--;
  (void)i++;
  if (i > len)
    {
      return DESCVALID_ERR_NO_OBJSIZE;
    }

  objsize = data[i++];
  if (objsize == 0)
    {
      // Object with no configuration is OK

      return static_cast<int>(i);
    }

  if (i > len)
    {
      return DESCVALID_ERR_NO_CFG_HEADER;
    }

  for (size_t j = 0; j < objsize; j++)
    {
      objcfg.v = data[i++];

      if (i > len)
        {
          // Check for zero lenght last element in config

          if (objcfg.s.size == 0 && j == objsize - 1)
            {
              continue;
            }

          return DESCVALID_ERR_CFG_TRUNCATED;
        }

      for (size_t k = 0; k < objcfg.s.size; k++)
        {
          (void)data[i++];
          if (i > len)
            {
              // Check for last element in config

              if (k == (size_t)(objcfg.s.size - 1))
                {
                  continue;
                }

              return DESCVALID_ERR_CFG_DATA;
            }
        }
    }

  // Return offset scaled to bytes

  if (i > INT_MAX)
    {
      return DESCVALID_ERR_LEN_ALIGN;
    }

  return static_cast<int>(i);
}

// Validate descriptor using default object validation logic.

int CObject::validateDesc(const uint32_t *desc, size_t len)
{
  size_t i = 0;

  while (len > 0)
    {
      int ret;

      ret = CObject::descValidDefault(&desc[i], len);

      if (ret > 0)
        {
          // Return value is scaled in 4B, scale back to bytes

          len -= ret * sizeof(uint32_t);
          i += ret;
        }
      else
        {
          DAWNERR("len=%d i=%d ret=%d\n", (int)len, (int)i, ret);
          return ret;
        }
    }

  return OK;
}

SObjectId::UObjectId CObject::getId() const
{
  return uobjid;
}

SObjectId::ObjectId CObject::getIdV() const
{
  return uobjid.v;
}

bool CObject::getCfgFlag() const
{
  return objdesc.getSize() > 0;
}

uint8_t CObject::getType() const
{
  return uobjid.s.type;
}

uint16_t CObject::getCls() const
{
  return uobjid.s.cls;
}

uint8_t CObject::getDtype() const
{
  return uobjid.s.dtype;
}

uint8_t CObject::getFlags() const
{
  return uobjid.s.flags;
}

uint16_t CObject::getPriv() const
{
  return uobjid.s.priv;
}

CDescObject &CObject::getDesc()
{
  return objdesc;
}

size_t CObject::getDtypeSize() const
{
  return SObjectId::getDtypeSize_((SObjectId::EObjectDataType)objdesc.getObjectDtype());
}

int CObject::setObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
{
  SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t l = SObjectCfg::objectCfgGetSize(objcfg);
  int ret;

  // Check if objectCfg is RW

  if (SObjectCfg::objectCfgGetRw(objcfg) == false)
    {
      DAWNERR("objectCfg is not RW: %" PRIx32 "\n", objcfg);
      return -EACCES;
    }

  // Verify data length

  if (len < l)
    {
      DAWNERR("invalid size: %" PRIx32 "\n", objcfg);
      return -EINVAL;
    }

  // Update data

  item = objdesc.objectCfgItemId(objcfg);
  if (!item)
    {
      return -EINVAL;
    }

  ret = onSetObjConfig(objcfg, data, l);
  if (ret < 0)
    {
      return ret;
    }

  std::memcpy(&item->data, data, l * sizeof(uint32_t));
  return OK;
}

int CObject::getObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
{
  SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t l = SObjectCfg::objectCfgGetSize(objcfg);

  // Verify data length

  if (len < l)
    {
      DAWNERR("invalid size: %" PRIx32 "\n", objcfg);
      return -EINVAL;
    }
  // Get data

  item = objdesc.objectCfgItemId(objcfg);
  if (!item)
    {
      return -EINVAL;
    }

  std::memcpy(data, &item->data, l * sizeof(uint32_t));

  return OK;
}
