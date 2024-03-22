// dawn/include/dawn/proto/nimble/iprph.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vector>

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/proto/nimble/adv.hxx"

namespace dawn
{
/**
 * @brief Interface for BLE peripheral services with GATT characteristics.
 *
 * This interface defines the callback and UUID constants for GATT services
 * exposed by the BLE peripheral.
 */

class IProtoNimblePrphCb
{
public:
  /**
   * @brief Automation I/O Service UUID (0x1815).
   *
   * Service for digital and analog I/O control and monitoring via BLE.
   */

  constexpr static uint32_t UUID_AIOS[4] = {0x18150010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit Bluetooth SIG UUID for AIOS. */

  constexpr static uint16_t UUID16_AIOS = 0x1815;

  /**
   * @brief AIOS Input Trigger Set characteristic (0x2A57).
   *
   * Specifies trigger conditions for digital inputs.
   */

  constexpr static uint32_t UUID_INPUT_TRIGGER_SET[4] = {0x2a570010,
                                                         0x7474754e,
                                                         0x694e2058,
                                                         0x454c426d};

  /**
   * @brief AIOS Output Trigger Set characteristic (0x2A5C).
   *
   * Specifies trigger conditions for digital outputs.
   */

  constexpr static uint32_t UUID_OUTPUT_TRIGGER_SET[4] = {0x2a5c0010,
                                                          0x7474754e,
                                                          0x694e2058,
                                                          0x454c426d};

  /**
   * @brief AIOS Time Trigger Set characteristic (0x2A3F).
   *
   * Specifies time-based trigger conditions.
   */

  constexpr static uint32_t UUID_TIME_TRIGGER_SET[4] = {0x2a3f0010,
                                                        0x7474754e,
                                                        0x694e2058,
                                                        0x454c426d};

  /**
   * @brief Environmental Sensing Service UUID (0x181A).
   *
   * Service for environmental measurement characteristics (temperature,
   * humidity, pressure, UV index, wind speed/direction).
   */

  constexpr static uint32_t UUID_ESS[4] = {0x181a0010, 0x7474754e, 0x694e2058, 0x454c426d};
  /** @brief 16-bit Bluetooth SIG UUID for ESS. */

  constexpr static uint16_t UUID16_ESS = 0x181a;

  /**
   * @brief Industrial Measurement Device Service UUID (0x185A).
   *
   * Service for industrial measurement devices and sensors.
   */

  constexpr static uint32_t UUID_IMDS[4] = {0x185a0010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit Bluetooth SIG UUID for IMDS. */

  constexpr static uint16_t UUID16_IMDS = 0x185a;

  /**
   * @brief Binary Sensor Service UUID (0x183B).
   *
   * Service for binary state sensors (on/off, open/closed).
   */

  constexpr static uint32_t UUID_BSS[4] = {0x183b0010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit Bluetooth SIG UUID for BSS. */

  constexpr static uint16_t UUID16_BSS = 0x183b;

  /**
   * @brief Digital Characteristic UUID (0x2A56).
   *
   * Represents digital (boolean) I/O values.
   */

  constexpr static uint32_t UUID_DIGITAL[4] = {0x2a560010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Digital characteristic. */

  constexpr static uint16_t UUID16_DIGITAL = 0x2a56;

  /**
   * @brief Analog Characteristic UUID (0x2A58).
   *
   * Represents analog (continuous) I/O values.
   */

  constexpr static uint32_t UUID_ANALOG[4] = {0x2a580010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Analog characteristic. */

  constexpr static uint16_t UUID16_ANALOG = 0x2a58;

  /**
   * @brief Aggregate Characteristic UUID (0x2A5A).
   *
   * Aggregates readable AIOS Digital and Analog characteristic values.
   */

  constexpr static uint32_t UUID_AGGREGATE[4] = {0x2a5a0010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Aggregate characteristic. */

  constexpr static uint16_t UUID16_AGGREGATE = 0x2a5a;

  /**
   * @brief Temperature Characteristic UUID (0x2A6E).
   *
   * Temperature measurement in 0.01°C units.
   */

  constexpr static uint32_t UUID_TEMP[4] = {0x2a6e0010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Temperature characteristic. */

  constexpr static uint16_t UUID16_TEMP = 0x2a6e;

  /**
   * @brief Acceleration Characteristic UUID (0x2C06).
   *
   * NOTE: NON-STANDARD. 0x2C06 is not assigned by the Bluetooth SIG; this
   * is a Dawn-vendor placeholder for three-axis acceleration.
   */

  constexpr static uint32_t UUID_ACCEL[4] = {0x2c060010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Acceleration characteristic. */

  constexpr static uint16_t UUID16_ACCEL = 0x2c06;

  /**
   * @brief Humidity Characteristic UUID (0x2A6F).
   *
   * Relative humidity in 0.01% units.
   */

  constexpr static uint32_t UUID_HUM[4] = {0x2a6f0010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Humidity characteristic. */

  constexpr static uint16_t UUID16_HUM = 0x2a6f;

  /**
   * @brief Pressure Characteristic UUID (0x2A6D).
   *
   * Barometric pressure in Pascals.
   */

  constexpr static uint32_t UUID_PRESS[4] = {0x2a6d0010, 0x7474754e, 0x694e2058, 0x454c426d};
  /** @brief 16-bit UUID for Pressure characteristic. */

  constexpr static uint16_t UUID16_PRESS = 0x2a6d;

  /**
   * @brief UV Index Characteristic UUID (0x2A76).
   *
   * UV index value.
   */

  constexpr static uint32_t UUID_UVIDX[4] = {0x2a760010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for UV Index characteristic. */

  constexpr static uint16_t UUID16_UVIDX = 0x2a76;

  /**
   * @brief True Wind Speed Characteristic UUID (0x2A70).
   *
   * Wind speed in 0.01 m/s units.
   */

  constexpr static uint32_t UUID_TWINDSPEED[4] = {0x2a700010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for True Wind Speed characteristic. */

  constexpr static uint16_t UUID16_TWINDSPEED = 0x2a70;

  /**
   * @brief True Wind Direction Characteristic UUID (0x2A71).
   *
   * Wind direction in degrees (0-359).
   */

  constexpr static uint32_t UUID_TWINDDIR[4] = {0x2a710010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for True Wind Direction characteristic. */

  constexpr static uint16_t UUID16_TWINDDIR = 0x2a71;

  /**
   * @brief Voltage Characteristic UUID (0x2B18).
   *
   * Electrical voltage measurement.
   */

  constexpr static uint32_t UUID_VOLTAGE[4] = {0x2b180010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Voltage characteristic. */

  constexpr static uint16_t UUID16_VOLTAGE = 0x2b18;

  /**
   * @brief Voltage Frequency Characteristic UUID (0x2BE8).
   *
   * AC voltage frequency in Hz.
   */

  constexpr static uint32_t UUID_VOLTAGEFREQ[4] = {0x2be80010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Voltage Frequency characteristic. */

  constexpr static uint16_t UUID16_VOLTAGEFREQ = 0x2be8;

  /**
   * @brief Voltage Specification Characteristic UUID (0x2B19).
   *
   * Rated voltage specification for the device.
   */

  constexpr static uint32_t UUID_VOLTAGESPEC[4] = {0x2b190010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Voltage Specification characteristic. */

  constexpr static uint16_t UUID16_VOLTAGESPEC = 0x2b19;

  /**
   * @brief Voltage Statistics Characteristic UUID (0x2B1A).
   *
   * Voltage measurements and statistics (min, max, average).
   */

  constexpr static uint32_t UUID_VOLTAGESTAT[4] = {0x2b1a0010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Voltage Statistics characteristic. */

  constexpr static uint16_t UUID16_VOLTAGESTAT = 0x2b1a;

  /**
   * @brief Electric Current Characteristic UUID (0x2AEE).
   *
   * Electrical current measurement in Amperes.
   */

  constexpr static uint32_t UUID_CURRENT[4] = {0x2aee0010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Electric Current characteristic. */

  constexpr static uint16_t UUID16_CURRENT = 0x2aee;

  /**
   * @brief Electric Current Range Characteristic UUID (0x2AEF).
   *
   * Supported current measurement range (min, max).
   */

  constexpr static uint32_t UUID_CURRENTRANGE[4] = {0x2aef0010, 0x7474754e, 0x694e2058, 0x454c426d};
  /** @brief 16-bit UUID for Electric Current Range characteristic. */

  constexpr static uint16_t UUID16_CURRENTRANGE = 0x2aef;

  /**
   * @brief Electric Current Specification Characteristic UUID (0x2AF0).
   *
   * Rated current specification for the device.
   */

  constexpr static uint32_t UUID_CURRENTSPEC[4] = {0x2af00010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Electric Current Specification characteristic. */

  constexpr static uint16_t UUID16_CURRENTSPEC = 0x2af0;

  /**
   * @brief Electric Current Statistics Characteristic UUID (0x2AF1).
   *
   * Current measurements and statistics (min, max, average).
   */

  constexpr static uint32_t UUID_CURRENTSTATS[4] = {0x2af10010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Electric Current Statistics characteristic. */

  constexpr static uint16_t UUID16_CURRENTSTATS = 0x2af1;

  /**
   * @brief Illuminance Characteristic UUID (0x2AFB).
   *
   * Illuminance / light level in 0.01 lux units (uint24).
   */

  constexpr static uint32_t UUID_ILLUMINANCE[4] = {0x2afb0010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Illuminance characteristic. */

  constexpr static uint16_t UUID16_ILLUMINANCE = 0x2afb;

  /**
   * @brief Electric Resistance Characteristic UUID (0x272A).
   *
   * NOTE: NON-STANDARD. 0x272A is not assigned by the Bluetooth SIG; this
   * is a Dawn-vendor characteristic exposed for gas-resistance sensors
   * until a real GATT mapping is chosen. Unit is kilohms (no scaling).
   */

  constexpr static uint32_t UUID_RESISTANCE[4] = {0x272a0010, 0x7474754e, 0x694e2058, 0x454c426d};

  /** @brief 16-bit UUID for Electric Resistance characteristic. */

  constexpr static uint16_t UUID16_RESISTANCE = 0x272a;

  /**
   * @brief Scaling factors for GATT characteristics.
   *
   * Maps characteristic UUIDs to their scaling factors.
   */

  constexpr static std::pair<uint16_t, float> charScale[] = {
    {UUID16_TEMP, 100.0f},        /** Temperature: 0.01°C units */

    {UUID16_HUM, 100.0f},         /** Humidity: 0.01% units */

    {UUID16_PRESS, 100.0f},       /** Pressure: 1 Pa units */

    {UUID16_RESISTANCE, 1.0f},    /** Resistance: 1 kilohm units (no scaling) */

    {UUID16_UVIDX, 1.0f},         /** UV Index: 1 unit (no scaling) */

    {UUID16_ILLUMINANCE, 100.0f}, /** Illuminance: 0.01 lux units */

  };

  /** @brief Number of characteristic scaling entries. */

  constexpr static size_t charScaleSize =
    (sizeof(IProtoNimblePrphCb::charScale) / sizeof(IProtoNimblePrphCb::charScale[0]));

  /**
   * @brief Get scaling factor for a characteristic UUID.
   *
   * Returns the scaling factor for converting raw values to proper units.
   *
   * @param[in] u 16-bit Bluetooth SIG UUID.
   * @return float Scaling factor (1.0 = no scaling, 100.0 = divide by 100).
   */

  static float charScaleGet(uint16_t u)
  {
    for (size_t i = 0; i < IProtoNimblePrphCb::charScaleSize; ++i)
      {
        if (IProtoNimblePrphCb::charScale[i].first == u)
          {
            return IProtoNimblePrphCb::charScale[i].second;
          }
      }

    return 0.0f;
  }

  /**
   * @brief Register a GATT service with the peripheral.
   *
   * Called by service implementations to register their GATT service
   * definition.
   *
   * @param[in] svc GATT service definition (from NimBLE ble_gatt_svc_def).
   * @return int Service ID assigned by peripheral (for start/stop), or.
   */

  virtual int serviceRegister(struct ble_gatt_svc_def *svc) = 0;

  /**
   * @brief Start a specific service.
   *
   * Enables a service (makes its characteristics available to clients).
   *
   * @param[in] id Service ID from serviceRegister().
   * @return int 0 on success, negative error code on failure.
   */

  virtual int startService(int id) = 0;

  /**
   * @brief Stop a specific service.
   *
   * Disables a service (removes its characteristics from the GATT database).
   *
   * @param[in] id Service ID from serviceRegister().
   * @return int 0 on success, negative error code on failure.
   */

  virtual int stopService(int id) = 0;

  /**
   * @brief Register an I/O object for this service.
   *
   * Called by services to register I/O objects.
   *
   * @param[in] id I/O object ID to register.
   */

  virtual void regObject(SObjectId::ObjectId id) = 0;

  /**
   * @brief Get protocol object by ID.
   *
   * Retrieves a protocol instance by its object ID.
   *
   * @param[in] id I/O object ID.
   * @return CIOCommon* Pointer to I/O object, nullptr if not registered.
   */

  virtual CIOCommon *getObject(SObjectId::ObjectId id) = 0;

  /**
   * @brief Get count of registered I/O objects.
   *
   * @return size_t Total number of I/O objects registered.
   */

  virtual size_t getObjectsLen() = 0;
};

/**
 * @brief Base interface for GATT services exposed by BLE peripheral.
 *
 * All BLE services (DIS, BAS, ESS, AIOS, IMDS) implement this interface.
 */

class IProtoNimblePrphService
{
public:
  /**
   * @brief Context data for characteristic notification callbacks.
   *
   * Used when I/O value changes trigger GATT notifications to BLE clients.
   */

  struct
  {
    CIOCommon *io;    /** Pointer to I/O object being monitored */

    uint16_t handle;  /** GATT attribute handle for notification */

    io_ddata_t *data; /** Latest I/O data for notification */

  } typedef SPrphNotiferCb;

  /**
   * @brief Constructor.
   *
   * Stores configuration descriptor and peripheral callback reference.
   *
   * @param[in] desc_ Configuration descriptor item for this service.
   * @param[in] cb_ Callback interface to peripheral.
   */

  IProtoNimblePrphService(const SObjectCfg::SObjectCfgItem *desc_, IProtoNimblePrphCb *cb_)
  {
    DAWNASSERT(desc_ != nullptr, "NULL pointer");
    DAWNASSERT(cb_ != nullptr, "NULL pointer");

    this->desc = desc_;
    this->cb = cb_;
  };

  /**
   * @brief Virtual destructor.
   *
   * Allows proper cleanup in derived classes.
   */

  virtual ~IProtoNimblePrphService() {};

  /**
   * @brief Initialize service.
   *
   * Allocates service resources and prepares characteristics.
   *
   * @return int 0 on success, negative error code on failure.
   */

  virtual int init() = 0;

  /**
   * @brief Deinitialize service.
   *
   * Releases all allocated resources for this service.
   *
   * @return int 0 on success, negative error code on failure.
   */

  virtual int deinit() = 0;

  /**
   * @brief Start service.
   *
   * Registers the service with the GATT database.
   *
   * @return int 0 on success, negative error code on failure.
   */

  virtual int start() = 0;

  /**
   * @brief Stop service.
   *
   * Removes the service from the GATT database.
   *
   * @return int 0 on success, negative error code on failure.
   */

  virtual int stop() = 0;

  /**
   * @brief Vector of I/O objects exposed by this service.
   *
   * Each entry is an I/O object ID that this service manages.
   */

  std::vector<SObjectId::ObjectId> vio;

  /**
   * @brief Callback interface to peripheral.
   *
   * Services use this to register GATT services, start/stop services, and
   * access I/O objects.
   */

  IProtoNimblePrphCb *cb;

  /**
   * @brief Configuration descriptor for this service.
   *
   * Points to the configuration item that specified this service's setup.
   */

  const SObjectCfg::SObjectCfgItem *desc;
};
} // Namespace dawn
