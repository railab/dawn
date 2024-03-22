// dawn/include/dawn/io/config.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <map>
#include <mutex>

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Configuration I/O for runtime object management.
 *
 * Provides a runtime-configurable interface to manage and configure other I/O
 * objects and programs.
 */

class CIOConfig : public CIOCommon
{
public:
  enum
  {
    IO_CONFIG_CFG_FIRST = 0,
    IO_CONFIG_CFG_ALLOC = 1,
    IO_CONFIG_CFG_OBJCFG = 2,
    IO_CONFIG_CFG_OFFSET = 3,
    IO_CONFIG_CFG_SIZE = 4,
    IO_CONFIG_CFG_LAST = 31
  };

  explicit CIOConfig(CDescObject &desc)
    : CIOCommon(desc)
    , objcfg(0)
    , cfg_offset(0)
    , cfg_size(0)
  {
  }

  ~CIOConfig() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "config";
  }
#endif

  int configure() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;
  size_t getDataSize() const override;
  size_t getDataDim() const override;

  bool isRead() const override
  {
    return true;
  };

  bool isWrite() const override
  {
    return true;
  };

  bool isNotify() const override
  {
    return false;
  };

  bool isBatch() const override
  {
    return false;
  };

  int bind(CObject *obj, uint32_t id);

  using ObjectIdHelper = CIOCommon::IOObjectIdHelperNoTS<IO_CLASS_CONFIG, SObjectId::DTYPE_UINT32>;

  constexpr static SObjectId::ObjectId objectId(SObjectId::EObjectDataType dtype, uint16_t inst)
  {
    return ObjectIdHelper::create(dtype, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdCfg()
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_CONFIG,
                                 SObjectId::DTYPE_UINT32,
                                 false,
                                 1,
                                 IO_CONFIG_CFG_OBJCFG);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdAlloc(uint8_t dtype, bool rw, uint8_t len)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, (CIOCommon::IO_CLASS_CONFIG), dtype, rw, len, IO_CONFIG_CFG_ALLOC);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdOffset()
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_CONFIG,
                                 SObjectId::DTYPE_UINT32,
                                 false,
                                 1,
                                 IO_CONFIG_CFG_OFFSET);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdSize()
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_CONFIG,
                                 SObjectId::DTYPE_UINT32,
                                 false,
                                 1,
                                 IO_CONFIG_CFG_SIZE);
  }

  std::map<uint32_t, CObject *> map; ///< Map of object ID to bound CObject pointers.

private:
  uint32_t objcfg;                   ///< Current target configuration ID.
  uint32_t cfg_offset;               ///< Word offset within target config field.
  uint32_t cfg_size;                 ///< Number of words exposed (0 = full field).

  void configureDesc(const CDescObject &desc);
};
} // Namespace dawn
