// dawn/include/dawn/prog/adjust.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;
class IIOCommon;
class io_ddata_t;

/**
 * @brief Data scaling and offset transformation PROG.
 *
 * Transforms input I/O data by applying scale and offset operations:
 *
 *   output = (input * scale) + offset
 *
 * Provides real-time value adjustment for sensor
 * calibration, unit conversion, and data range normalization.
 */

class CProgAdjust : public CProgCommon
{
public:
  enum
  {
    PROG_ADJUST_CFG_FIRST = 0,  ///< reserved
    PROG_ADJUST_CFG_IOBIND = 1, ///< I/O binding configuration
    PROG_ADJUST_CFG_PARAMS = 2, ///< Adjustment parameters (scale, offset)
    PROG_ADJUST_CFG_LAST = 31   ///< reserved
  };

  struct
  {
    SObjectId::UObjectId objid;  ///< Source I/O object ID
    SObjectId::UObjectId output; ///< Output I/O object ID
  } typedef SProgAdjustIOBind;

  struct
  {
    uint32_t offset; ///< Offset added to scaled input
    uint32_t scale;  ///< Multiplication factor/scale
  } typedef SProgAdjustParams;

  explicit CProgAdjust(CDescObject &desc)
    : CProgCommon(desc)
    , src(nullptr)
    , output(nullptr)
    , cfg(nullptr)
    , ioData(nullptr)
    , outputData(nullptr)
    , ioscale(0)
    , iooffset(0)
    , srcType(SObjectId::DTYPE_ANY)
    , outputType(SObjectId::DTYPE_ANY)
  {
  }

  ~CProgAdjust() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "adjust";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len) override;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_ADJUST, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_ADJUST, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(size_t i = 1)
  {
    return CProgAdjust::cfgId(false, 1 + i, PROG_ADJUST_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgParams(bool rw = false)
  {
    return CProgAdjust::cfgId(rw, 2, PROG_ADJUST_CFG_PARAMS);
  }

private:
  CIOCommon *src;         // @Source I/O object.
  CIOCommon *output;      // @Result output I/O object.
  SProgAdjustIOBind *cfg; // Configuration structure reference.
  io_ddata_t *ioData;     // Input data buffer.
  io_ddata_t *outputData; // Output data buffer.
  uint32_t ioscale;       // Stored scale parameter.
  uint32_t iooffset;      // Stored offset parameter.
  int srcType;            // Source dtype.
  int outputType;         // Result dtype.

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int allocObject(SProgAdjustIOBind *alloc);
  int bindPrepare();
  bool isSupportedConvType(int sourceType, int targetType);
  int refresh();
  void handle(io_ddata_t *data);
};
} // Namespace dawn
