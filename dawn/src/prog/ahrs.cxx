// dawn/src/prog/ahrs.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/ahrs.hxx"

#include <ctime>
#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"

using namespace dawn;

namespace
{
constexpr float GRAVITY = 9.80665f;
constexpr float RAD2DEG = 57.29577951308232f;
constexpr float DT_MIN = 0.001f; // clamp window for measured dt
constexpr float DT_MAX = 0.1f;

uint64_t monotonicNs()
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1000000000ull + ts.tv_nsec;
}
} // namespace

CProgAhrs::CProgAhrs(CDescObject &desc)
  : CProgCommon(desc)
  , accel(nullptr)
  , gyro(nullptr)
  , mag(nullptr)
  , output(nullptr)
  , accelId(0)
  , gyroId(0)
  , magId(0)
  , outputId(0)
  , outData(nullptr)
  , active(false)
  , registered(false)
  , pGain(0.5f)
  , pRejection(10.0f)
  , pPeriod(5.0f)
  , pRate(50.0f)
  , haveAccel(false)
  , haveMag(false)
  , lastNs(0)
{
}

CProgAhrs::~CProgAhrs()
{
  deinit();
}

int CProgAhrs::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_AHRS)
        {
          DAWNERR("ahrs: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_AHRS_CFG_ACCEL:
            {
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              accelId = ids[0].v;
              setObjectMapItem(accelId, nullptr);
              break;
            }

          case PROG_AHRS_CFG_GYRO:
            {
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              gyroId = ids[0].v;
              setObjectMapItem(gyroId, nullptr);
              break;
            }

          case PROG_AHRS_CFG_MAG:
            {
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              magId = ids[0].v;
              setObjectMapItem(magId, nullptr);
              break;
            }

          case PROG_AHRS_CFG_OUTPUT:
            {
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              outputId = ids[0].v;
              setObjectMapItem(outputId, nullptr);
              break;
            }

          case PROG_AHRS_CFG_PARAMS:
            {
              if (item->cfgid.s.size != sizeof(SProgAhrsParams) / 4)
                {
                  DAWNERR("ahrs: invalid PARAMS size %d, expected %zu\n",
                          item->cfgid.s.size,
                          sizeof(SProgAhrsParams) / 4);
                  return -EINVAL;
                }

              const SProgAhrsParams *params =
                reinterpret_cast<const SProgAhrsParams *>(item->data);

              pGain = SObjectCfg::cfgToF(params->gain);
              pRejection = SObjectCfg::cfgToF(params->accel_rejection);
              pPeriod = SObjectCfg::cfgToF(params->recovery_period);
              pRate = SObjectCfg::cfgToF(params->rate);
              break;
            }

          default:
            DAWNERR("ahrs: unsupported cfg id %u\n", item->cfgid.s.id);
            return -EINVAL;
        }
    }

  if (accelId == 0 || gyroId == 0 || outputId == 0)
    {
      DAWNERR("ahrs: accel, gyro and output are required\n");
      return -EINVAL;
    }

  if (!paramsValid(pGain, pRejection, pPeriod, pRate))
    {
      DAWNERR("ahrs: params out of range\n");
      return -EINVAL;
    }

  return OK;
}

int CProgAhrs::configure()
{
  return configureDesc(getDesc());
}

bool CProgAhrs::paramsValid(float gain, float rejection, float period, float rate)
{
  return gain >= 0.0f && gain <= 10.0f && rejection >= 0.0f && rejection <= 90.0f &&
         period >= 0.0f && period <= 60.0f && rate >= 1.0f && rate <= 1000.0f;
}

int CProgAhrs::validateInput(CIOCommon *io) const
{
  if (io == nullptr)
    {
      return -EIO;
    }

  if (!io->isRead() || !io->isNotify() || io->getDtype() != SObjectId::DTYPE_FLOAT ||
      io->getDataDim() < 3)
    {
      return -EINVAL;
    }

  return OK;
}

void CProgAhrs::applySettings()
{
  FusionAhrsSettings settings;

  settings.convention = FusionConventionNwu;
  settings.gain = pGain;
  settings.gyroscopeRange = 2000.0f;
  settings.accelerationRejection = pRejection;
  // The optional magnetometer shares the rejection knob with the accel.
  settings.magneticRejection = (magId != 0) ? pRejection : 0.0f;
  settings.recoveryTriggerPeriod = static_cast<unsigned int>(pPeriod * pRate);

  FusionAhrsSetSettings(&ahrs, &settings);
}

int CProgAhrs::init()
{
  int ret;

  accel = getIO(accelId);
  gyro = getIO(gyroId);
  output = getIO(outputId);

  if (accel == nullptr || gyro == nullptr || output == nullptr)
    {
      DAWNERR("ahrs: IO not found\n");
      return -EIO;
    }

  ret = validateInput(accel);
  if (ret != OK)
    {
      DAWNERR("ahrs: accel IO 0x%" PRIx32 " incompatible\n", accelId);
      return ret;
    }

  ret = validateInput(gyro);
  if (ret != OK)
    {
      DAWNERR("ahrs: gyro IO 0x%" PRIx32 " incompatible\n", gyroId);
      return ret;
    }

  if (magId != 0)
    {
      mag = getIO(magId);
      ret = validateInput(mag);
      if (ret != OK)
        {
          DAWNERR("ahrs: mag IO 0x%" PRIx32 " incompatible\n", magId);
          return ret;
        }
    }

  ret = prepareWritableTarget(output, 3, true);
  if (ret != OK)
    {
      DAWNERR("ahrs: output prepare failed %d\n", ret);
      return ret;
    }

  outData = output->ddata_alloc(1);
  if (outData == nullptr)
    {
      DAWNERR("ahrs: data allocation failed\n");
      return -ENOMEM;
    }

  FusionOffsetInitialise(&offset, static_cast<unsigned int>(pRate));
  FusionAhrsInitialise(&ahrs);
  applySettings();
  haveAccel = false;
  haveMag = false;
  lastNs = 0;

  return OK;
}

int CProgAhrs::deinit()
{
  doStop();
  delete outData;
  outData = nullptr;
  accel = nullptr;
  gyro = nullptr;
  mag = nullptr;
  output = nullptr;
  accelId = 0;
  gyroId = 0;
  magId = 0;
  outputId = 0;
  return OK;
}

int CProgAhrs::accelNotifierCb(void *priv, io_ddata_t *data)
{
  CProgAhrs *obj = static_cast<CProgAhrs *>(priv);

  if (obj == nullptr || !obj->active || data == nullptr)
    {
      return OK;
    }

  const float *v = reinterpret_cast<const float *>(data->getDataPtr());

  obj->accelG.axis.x = v[0] / GRAVITY;
  obj->accelG.axis.y = v[1] / GRAVITY;
  obj->accelG.axis.z = v[2] / GRAVITY;
  obj->haveAccel = true;
  return OK;
}

int CProgAhrs::magNotifierCb(void *priv, io_ddata_t *data)
{
  CProgAhrs *obj = static_cast<CProgAhrs *>(priv);

  if (obj == nullptr || !obj->active || data == nullptr)
    {
      return OK;
    }

  const float *v = reinterpret_cast<const float *>(data->getDataPtr());

  obj->magRaw.axis.x = v[0];
  obj->magRaw.axis.y = v[1];
  obj->magRaw.axis.z = v[2];
  obj->haveMag = true;
  return OK;
}

int CProgAhrs::gyroNotifierCb(void *priv, io_ddata_t *data)
{
  CProgAhrs *obj = static_cast<CProgAhrs *>(priv);

  if (obj == nullptr || !obj->active || data == nullptr)
    {
      return OK;
    }

  obj->handleGyro(data);
  return OK;
}

void CProgAhrs::handleGyro(io_ddata_t *data)
{
  if (!haveAccel || outData == nullptr || output == nullptr)
    {
      // No attitude reference yet - do not integrate.

      return;
    }

  const float *v = reinterpret_cast<const float *>(data->getDataPtr());
  FusionVector g;

  g.axis.x = v[0] * RAD2DEG;
  g.axis.y = v[1] * RAD2DEG;
  g.axis.z = v[2] * RAD2DEG;

  // dt from the monotonic clock; fall back to the nominal rate outside the
  // plausible window (first sample, scheduling hiccup, paused stream).

  uint64_t now = monotonicNs();
  float dt = 1.0f / pRate;

  if (lastNs != 0)
    {
      float measured = static_cast<float>(now - lastNs) * 1e-9f;
      if (measured >= DT_MIN && measured <= DT_MAX)
        {
          dt = measured;
        }
    }

  lastNs = now;

  g = FusionOffsetUpdate(&offset, g);
  if (haveMag)
    {
      FusionAhrsUpdate(&ahrs, g, accelG, magRaw, dt);
    }
  else
    {
      FusionAhrsUpdateNoMagnetometer(&ahrs, g, accelG, dt);
    }

  const FusionVector earth = FusionAhrsGetEarthAcceleration(&ahrs);
  float *out = reinterpret_cast<float *>(outData->getDataPtr());

  out[0] = earth.axis.x * GRAVITY;
  out[1] = earth.axis.y * GRAVITY;
  out[2] = earth.axis.z * GRAVITY;

  int ret = output->setData(*outData);
  if (ret != OK)
    {
      DAWNERR("ahrs: output setData failed %d\n", ret);
    }
}

int CProgAhrs::onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
{
  if (SObjectCfg::objectCfgGetId(objcfg) != PROG_AHRS_CFG_PARAMS)
    {
      return OK;
    }

  if (data == nullptr || len != sizeof(SProgAhrsParams) / sizeof(uint32_t))
    {
      return -EINVAL;
    }

  const SProgAhrsParams *params = reinterpret_cast<const SProgAhrsParams *>(data);
  float gain = SObjectCfg::cfgToF(params->gain);
  float rejection = SObjectCfg::cfgToF(params->accel_rejection);
  float period = SObjectCfg::cfgToF(params->recovery_period);
  float rate = SObjectCfg::cfgToF(params->rate);

  if (!paramsValid(gain, rejection, period, rate))
    {
      return -EINVAL;
    }

  bool rateChanged = rate != pRate;

  pGain = gain;
  pRejection = rejection;
  pPeriod = period;
  pRate = rate;
  applySettings();

  if (rateChanged)
    {
      FusionOffsetInitialise(&offset, static_cast<unsigned int>(pRate));
    }

  return OK;
}

int CProgAhrs::doStart()
{
  int ret;

  if (!registered)
    {
      ret = accel->setNotifier(accelNotifierCb, 0, this);
      if (ret != OK)
        {
          DAWNERR("ahrs: accel setNotifier failed %d\n", ret);
          return ret;
        }

      ret = gyro->setNotifier(gyroNotifierCb, 0, this);
      if (ret != OK)
        {
          DAWNERR("ahrs: gyro setNotifier failed %d\n", ret);
          accel->setNotifier(nullptr, 0, nullptr);
          return ret;
        }

      if (mag != nullptr)
        {
          ret = mag->setNotifier(magNotifierCb, 0, this);
          if (ret != OK)
            {
              DAWNERR("ahrs: mag setNotifier failed %d\n", ret);
              accel->setNotifier(nullptr, 0, nullptr);
              gyro->setNotifier(nullptr, 0, nullptr);
              return ret;
            }
        }

      registered = true;
    }

  active = true;
  return OK;
}

int CProgAhrs::doStop()
{
  if (registered)
    {
      if (accel != nullptr)
        {
          accel->setNotifier(nullptr, 0, nullptr);
        }

      if (gyro != nullptr)
        {
          gyro->setNotifier(nullptr, 0, nullptr);
        }

      if (mag != nullptr)
        {
          mag->setNotifier(nullptr, 0, nullptr);
        }

      registered = false;
    }

  active = false;
  return OK;
}

bool CProgAhrs::hasThread() const
{
  return false;
}
