// dawn/include/dawn/io/encoder.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/porting/encoder.hxx"

namespace dawn
{
/** @brief Quadrature encoder I/O (position only). */

class CIOEncoder : public CIOCommon
{
public:
  enum
  {
    IO_ENCODER_CFG_FIRST = 0,
    IO_ENCODER_CFG_POSMAX = 1,
    IO_ENCODER_CFG_LAST = 31
  };

  explicit CIOEncoder(CDescObject &desc)
    : CIOCommon(desc)
    , fd(-1)
    , posmax(0)
    , hasPosmax(false)
  {
  }

  ~CIOEncoder() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "encoder";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int trigger(uint8_t cmd) override;

  size_t getDataSize() const override
  {
    return sizeof(int32_t);
  }

  size_t getDataDim() const override
  {
    return 1;
  }

  bool isRead() const override
  {
    return true;
  }

  bool isWrite() const override
  {
    return false;
  }

  bool isNotify() const override
  {
    return false;
  }

  bool isBatch() const override
  {
    return false;
  }

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_ENCODER, SObjectId::DTYPE_INT32>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_ENCODER, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPosmax()
  {
    return CIOEncoder::cfgId(false, SObjectId::DTYPE_UINT32, 1, IO_ENCODER_CFG_POSMAX);
  }

private:
  char path[PATH_MAX] = {};
  int fd;
  uint32_t posmax;
  bool hasPosmax;

  int configureDesc(const CDescObject &desc);
};

} // Namespace dawn
