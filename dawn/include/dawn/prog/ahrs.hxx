// dawn/include/dawn/prog/ahrs.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <inttypes.h>

#include "Fusion/Fusion.h"
#include "dawn/io/ddata.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
class CIOCommon;

/**
 * @brief AHRS sensor fusion (x-io Fusion): accel + gyro IOs in, world-frame
 *        linear acceleration (gravity removed, Z up, m/s^2) out.
 */
class CProgAhrs : public CProgCommon
{
public:
  enum
  {
    PROG_AHRS_CFG_FIRST = 0,
    PROG_AHRS_CFG_ACCEL = 1,
    PROG_AHRS_CFG_GYRO = 2,
    PROG_AHRS_CFG_OUTPUT = 3,
    PROG_AHRS_CFG_PARAMS = 4,
    PROG_AHRS_CFG_MAG = 5,
    PROG_AHRS_CFG_LAST = 31
  };

  struct
  {
    uint32_t gain;            ///< Filter gain (float bits)
    uint32_t accel_rejection; ///< Acceleration rejection threshold, deg (float bits)
    uint32_t recovery_period; ///< Recovery trigger period, s (float bits)
    uint32_t rate;            ///< Nominal sample rate, Hz (float bits)
  } typedef SProgAhrsParams;

  explicit CProgAhrs(CDescObject &desc);
  ~CProgAhrs() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "ahrs";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;
  bool hasThread() const override;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_AHRS, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_AHRS, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdAccel()
  {
    return CProgAhrs::cfgId(false, 1, PROG_AHRS_CFG_ACCEL);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdGyro()
  {
    return CProgAhrs::cfgId(false, 1, PROG_AHRS_CFG_GYRO);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdMag()
  {
    return CProgAhrs::cfgId(false, 1, PROG_AHRS_CFG_MAG);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdOutput()
  {
    return CProgAhrs::cfgId(false, 1, PROG_AHRS_CFG_OUTPUT);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgParams(bool rw = false)
  {
    return CProgAhrs::cfgId(rw, sizeof(SProgAhrsParams) / 4, PROG_AHRS_CFG_PARAMS);
  }

protected:
  int onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len) override;

private:
  CIOCommon *accel;
  CIOCommon *gyro;
  CIOCommon *mag; ///< Optional magnetometer input (nullptr when unbound)
  CIOCommon *output;
  SObjectId::ObjectId accelId;
  SObjectId::ObjectId gyroId;
  SObjectId::ObjectId magId;
  SObjectId::ObjectId outputId;
  io_ddata_t *outData;
  bool active;
  bool registered;

  // Cached params (decoded floats).

  float pGain;
  float pRejection;
  float pPeriod;
  float pRate;

  // Filter state.

  FusionAhrs ahrs;
  FusionOffset offset;
  FusionVector accelG; ///< Latest accel sample converted to g
  FusionVector magRaw; ///< Latest magnetometer sample (normalised by the lib)
  bool haveAccel;
  bool haveMag;
  uint64_t lastNs;     ///< Monotonic time of the previous gyro sample

  static int accelNotifierCb(void *priv, io_ddata_t *data);
  static int gyroNotifierCb(void *priv, io_ddata_t *data);
  static int magNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int validateInput(CIOCommon *io) const;
  void applySettings();
  void handleGyro(io_ddata_t *data);
  static bool paramsValid(float gain, float rejection, float period, float rate);
};
} // namespace dawn
