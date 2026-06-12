// dawn/include/dawn/io/common.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdlib>

#include "dawn/common/object.hxx"
#include "dawn/debug.hxx"
#include "dawn/io/ddata.hxx"
#include "dawn/io/inotifier.hxx"
#include "dawn/io/limits.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Base class for all I/O objects.
 *
 * Provides common interface for data read/write operations, capability
 * queries, notifications, and configuration management.
 */

class CIOCommon : public CObject
{
public:
#ifdef CONFIG_DAWN_IO_HAS_STATS
  /**
   * @brief I/O runtime statistics.
   *
   * Tracks operation counts for I/O objects.
   */

  struct SIOStats
  {
    uint32_t read_count;  ///< Number of successful read operations
    uint32_t write_count; ///< Number of successful write operations
    uint32_t error_count; ///< Number of failed operations
  } typedef IOStats;

  /**
   * @brief Get runtime statistics.
   *
   * @return Const reference to statistics structure.
   */

  const IOStats &getStats() const
  {
    return stats;
  }

  /**
   * @brief Reset statistics counters.
   */

  void resetStats()
  {
    stats.read_count = 0;
    stats.write_count = 0;
    stats.error_count = 0;
  }
#endif // CONFIG_DAWN_IO_HAS_STATS

  /**
   * @brief I/O common flags.
   *
   * IO_FLAGS_NONE: No special flags - IO_FLAGS_TS: I/O supports timestamp (set
   * in flags field of ObjectID).
   */

  enum
  {
    IO_FLAGS_NONE = 0,      ///< No flags
    IO_FLAGS_TS = (1 << 0), ///< Timestamp support flag

    /** @brief No more fit now in flags !! */

  } typedef EIOFlags;

  /** @brief I/O object class types. */

  enum
  {
    IO_CLASS_ANY = 0, ///< Any I/O class

    // Special purpose IOs

    IO_CLASS_CONFIG = 1,         ///< Configuration I/O
    IO_CLASS_TRIGGER = 2,        ///< Trigger I/O
    IO_CLASS_DESCRIPTOR = 3,     ///< Descriptor I/O
    IO_CLASS_CONTROL = 4,        ///< Control I/O
    IO_CLASS_CAPABILITIES = 53,  ///< Capabilities bitmask I/O
    IO_CLASS_DESC_SELECTOR = 54, ///< Descriptor slot selector I/O

    // Dummy

    IO_CLASS_DUMMY = 5, ///< Dummy I/O (for testing)

    // Number generators

    IO_CLASS_TIMESTAMP = 6,    ///< Timestamp generator
    IO_CLASS_RAND = 7,         ///< Random number generator
    IO_CLASS_DUMMY_NOTIFY = 8, ///< Timer-driven dummy IO

    // File I/O

    IO_CLASS_FILE = 9, ///< File system I/O

    // Sensors

    IO_CLASS_SENSOR_ACCELEROMETER = 10, ///< Accelerometer sensor
    IO_CLASS_SENSOR_MAGNETICFIELD = 11, ///< Magnetic field sensor
    IO_CLASS_SENSOR_GYROSCOPE = 12,     ///< Gyroscope sensor
    IO_CLASS_SENSOR_LIGHT = 13,         ///< Light sensor
    IO_CLASS_SENSOR_BAROMETER = 14,     ///< Barometer sensor
    IO_CLASS_SENSOR_PROXIMITY = 15,     ///< Proximity sensor
    IO_CLASS_SENSOR_HUMIDITY = 16,      ///< Humidity sensor
    IO_CLASS_SENSOR_TEMPERATURE = 17,   ///< Temperature sensor
    IO_CLASS_SENSOR_ATEMPERATURE = 18,  ///< Ambient temperature sensor
    IO_CLASS_SENSOR_RGB = 19,           ///< RGB color sensor
    IO_CLASS_SENSOR_IR = 20,            ///< Infrared sensor
    IO_CLASS_SENSOR_UV = 21,            ///< Ultraviolet sensor
    IO_CLASS_SENSOR_GAS = 22,           ///< Gas sensor

    // Sensor producers

    IO_CLASS_SENSOR_PRODUCER_ACCELEROMETER = 23, ///< Accelerometer publisher
    IO_CLASS_SENSOR_PRODUCER_MAGNETICFIELD = 24, ///< Magnetic field publisher
    IO_CLASS_SENSOR_PRODUCER_GYROSCOPE = 25,     ///< Gyroscope publisher
    IO_CLASS_SENSOR_PRODUCER_LIGHT = 26,         ///< Light publisher
    IO_CLASS_SENSOR_PRODUCER_BAROMETER = 27,     ///< Barometer publisher
    IO_CLASS_SENSOR_PRODUCER_PROXIMITY = 28,     ///< Proximity publisher
    IO_CLASS_SENSOR_PRODUCER_HUMIDITY = 29,      ///< Humidity publisher
    IO_CLASS_SENSOR_PRODUCER_TEMPERATURE = 30,   ///< Temperature publisher
    IO_CLASS_SENSOR_PRODUCER_ATEMPERATURE = 31,  ///< Ambient temperature publisher
    IO_CLASS_SENSOR_PRODUCER_RGB = 32,           ///< RGB color publisher
    IO_CLASS_SENSOR_PRODUCER_IR = 33,            ///< Infrared publisher
    IO_CLASS_SENSOR_PRODUCER_UV = 34,            ///< Ultraviolet publisher
    IO_CLASS_SENSOR_PRODUCER_GAS = 35,           ///< Gas publisher
    IO_CLASS_SENSOR_PRODUCER_CONFIG = 36,        ///< Sensor producer configuration IDs

    // GNSS sensor (uORB sensor_gnss). Read-only, so it has no producer
    // counterpart and lives outside the aligned sensor/producer ranges above.

    IO_CLASS_SENSOR_GNSS = 37,            ///< GNSS position+velocity (lat/lon/alt/speed/course)
    IO_CLASS_SENSOR_GNSS_TIME = 38,       ///< GNSS UTC time (seconds since epoch)
    IO_CLASS_SENSOR_GNSS_INFO = 39,       ///< GNSS accuracy + DOP (eph/epv/hdop/pdop/vdop)
    IO_CLASS_SENSOR_GNSS_SATELLITES = 40, ///< GNSS satellites used in fix

    // there is 59 sensor types in Nuttx now, most likely it grows later
    // we still have many free slots, but in the future, it there are no free IO slots
    // we can thing about combining sensor and sensor producer

    // System information

    IO_CLASS_SYSTEM_UPTIME = 41,     ///< System uptime
    IO_CLASS_SYSTEM_CPULOAD = 42,    ///< CPU load
    IO_CLASS_SYSTEM_RESET = 43,      ///< System reset control
    IO_CLASS_SYSTEM_RESETCAUSE = 44, ///< Reset cause
    IO_CLASS_SYSTEM_POWEROFF = 45,   ///< Power off control
    IO_CLASS_SYSTEM_HOSTNAME = 46,   ///< System hostname
    IO_CLASS_SYSTEM_UUID = 47,       ///< UUID
    IO_CLASS_SYSTEM_SYSTEMTIME = 48, ///< System time

    // Slots 49-59 reserved for future system classes (serial number,
    // firmware version, firmware update, etc.).

    // GPIO

    IO_CLASS_GPI_SINGLE = 60, ///< Single GPIO input
    IO_CLASS_GPO_SINGLE = 61, ///< Single GPIO output
    IO_CLASS_BUTTONS = 62,    ///< Button input
    IO_CLASS_LEDS = 63,       ///< LED output
    IO_CLASS_RGBLED = 64,     ///< RGB LED output

    // PWM / pulse output

    IO_CLASS_PWM = 78,        ///< PWM output
    IO_CLASS_PULSECOUNT = 79, ///< Finite pulse-train output

    // ADC/DAC

    IO_CLASS_DAC = 81,        ///< Digital-to-analog converter
    IO_CLASS_ADC_FETCH = 82,  ///< ADC fetch (on-demand)
    IO_CLASS_ADC_SYNC = 83,   ///< ADC sync (HW-triggered control loop)
    IO_CLASS_ADC_STREAM = 84, ///< ADC stream (batch/high-throughput)

    // Ecnoder input

    IO_CLASS_ENCODER = 85,       ///< Quadrature encoder (position)
    IO_CLASS_ENCODER_INDEX = 86, ///< Quadrature encoder (position+index)

    // Battery (fuel gauge)

    IO_CLASS_BATTERY_VOLTAGE = 90, ///< Battery voltage (fuel gauge)
    IO_CLASS_BATTERY_SOC = 91,     ///< Battery state of charge (%)
    IO_CLASS_BATTERY_STATE = 92,   ///< Battery charge state

    // Cellular modem

    IO_CLASS_LTE_SIGNAL = 93, ///< LTE signal quality (RSRP/RSRQ/SINR/RSSI)

    // Virtual IO

    IO_CLASS_VIRT = 100, ///< Virtual I/O

    // User defined

    IO_CLASS_USER_START = 500, ///< User-defined types start here
    IO_CLASS_USER,             ///< User-defined I/O

    IO_CLASS_LAST              ///< Last I/O class marker
  } typedef EIOClass;

  static_assert(IO_CLASS_LAST - 1 <= SObjectId::CLS_MAX);

  /**
   * @brief I/O common configuration IDs.
   *
   * Standard configuration identifiers for I/O objects:
   * - IO_CFG_DEVNO: Device number configuration
   * - IO_CFG_LIMIT_*: I/O data limits (min/max/step).
   */

  enum
  {
    IO_CFG_FIRST = 0,      ///< First config ID
    IO_CFG_DEVNO = 1,      ///< Device number configuration
    IO_CFG_LIMIT_MIN = 2,  ///< Minimum limit words
    IO_CFG_LIMIT_MAX = 3,  ///< Maximum limit words
    IO_CFG_LIMIT_STEP = 4, ///< Step limit words
    IO_CFG_NOTIFY = 5,     ///< Notifier configuration (type + priority)
    IO_CFG_LAST = 31       ///< Last config ID
  };

  /** @brief Notifier type enumeration. */

  enum
  {
    IO_NOTIFY_POLL = 0,   ///< Poll-based notifier (default)
    IO_NOTIFY_STREAM = 1, ///< Stream notifier (blocking read, one per IO)
  } typedef EIONotifyType;

  /**
   * @brief Template helper for creating ObjectIDs for I/O types.
   *
   * @tparam CLASS_ID The I/O class identifier (e.g., IO_CLASS_ADC_FETCH)
   * @tparam DEFAULT_DTYPE Default data type for this I/O
   */

  template<uint16_t CLASS_ID, uint8_t DEFAULT_DTYPE>
  class IOObjectIdHelper
  {
  public:
    /**
     * @brief Create ObjectID with default data type.
     *
     * @param ts Enable timestamp support if true.
     * @param inst Instance number (0 to MAX_INSTANCE-1).
     * @return ObjectID for the I/O instance.
     */

    constexpr static SObjectId::ObjectId create(bool ts, uint16_t inst)
    {
      uint8_t flags = 0;

#ifdef CONFIG_DAWN_IO_TIMESTAMP
      if (ts)
        {
          flags |= CIOCommon::IO_FLAGS_TS;
        }
#else
      DAWNASSERT(ts == false, "ts not supported");
#endif

      return SObjectId::objectId(SObjectId::OBJTYPE_IO, CLASS_ID, DEFAULT_DTYPE, flags, inst);
    }

    /**
     * @brief Create ObjectID with custom data type.
     *
     * @param dtype Data type for this instance.
     * @param ts Enable timestamp support if true.
     * @param inst Instance number (0 to MAX_INSTANCE-1).
     * @return ObjectID for the I/O instance.
     */

    constexpr static SObjectId::ObjectId create(SObjectId::EObjectDataType dtype,
                                                bool ts,
                                                uint16_t inst)
    {
      uint8_t flags = 0;

#ifdef CONFIG_DAWN_IO_TIMESTAMP
      if (ts)
        {
          flags |= CIOCommon::IO_FLAGS_TS;
        }
#else
      DAWNASSERT(ts == false, "ts not supported");
#endif

      return SObjectId::objectId(SObjectId::OBJTYPE_IO, CLASS_ID, dtype, flags, inst);
    }
  };

  /**
   * @brief Template helper for creating ObjectIDs for I/O types without
   * timestamp support.
   *
   * Simplified helper for I/O types that don't support timestamps.
   * Uses flags=0 (no timestamp flag).
   *
   * @tparam CLASS_ID The I/O class identifier
   * @tparam DEFAULT_DTYPE Default data type for this I/O
   */

  template<uint16_t CLASS_ID, uint8_t DEFAULT_DTYPE>
  class IOObjectIdHelperNoTS
  {
  public:
    /**
     * @brief Create ObjectID with fixed data type and no timestamp.
     *
     * @param inst Instance number (0 to MAX_INSTANCE-1).
     * @return ObjectID for the I/O instance.
     */

    constexpr static SObjectId::ObjectId create(uint16_t inst)
    {
      return IOObjectIdHelper<CLASS_ID, DEFAULT_DTYPE>::create(false, inst);
    }

    /**
     * @brief Create ObjectID with custom data type and no timestamp.
     *
     * @param dtype Data type for this instance.
     * @param inst Instance number (0 to MAX_INSTANCE-1).
     * @return ObjectID for the I/O instance.
     */

    constexpr static SObjectId::ObjectId create(SObjectId::EObjectDataType dtype, uint16_t inst)
    {
      return IOObjectIdHelper<CLASS_ID, DEFAULT_DTYPE>::create(dtype, false, inst);
    }
  };

  /**
   * @brief Construct CIOCommon from descriptor.
   *
   * @param desc Descriptor object defining this I/O's configuration.
   */

  explicit CIOCommon(CDescObject &desc);

  /**
   * @brief Get data from I/O (public interface with stats tracking).
   *
   * For seekable IOs always calls getDataAtImpl() (handles both offset=0 and
   * offset>0, copies min(buf_size, available) bytes).
   * For non-seekable IOs calls getDataImpl() when offset=0; offset>0 returns
   * -ENOTSUP.
   *
   * @param data Reference to IODataCmn structure to receive data.
   * @param len Number of data items to read (must be 1 for seekable IOs).
   * @param offset Byte offset for seekable IOs (0 for standard access).
   * @return OK on success, negative error code on failure.
   */

  int getData(IODataCmn &data, size_t len, size_t offset = 0)
  {
    int ret;

#ifdef CONFIG_DAWN_IO_SEEKABLE
    ret = isSeekable() ? getDataAtImpl(data, len, offset)
                       : (offset == 0 ? getDataImpl(data, len) : -ENOTSUP);
#else
    ret = (offset == 0) ? getDataImpl(data, len) : -ENOTSUP;
#endif
#ifdef CONFIG_DAWN_IO_HAS_STATS
    if (ret == OK)
      {
        if (offset == 0)
          {
            stats.read_count++;
          }
      }
    else if (ret != -ENOTSUP)
      {
        stats.error_count++;
      }
#endif
    return ret;
  }

  /**
   * @brief Set data for I/O (public interface with stats tracking).
   *
   * For seekable IOs always calls setDataAtImpl() (handles both offset=0 and
   * offset>0).
   * For non-seekable IOs calls setDataImpl() when offset=0; offset>0 returns
   * -ENOTSUP.
   *
   * @param data Reference to IODataCmn structure containing data.
   * @param offset Byte offset for seekable IOs (0 for standard access).
   * @return OK on success, negative error code on failure.
   */

  int setData(IODataCmn &data, size_t offset = 0)
  {
    int ret;

#ifdef CONFIG_DAWN_IO_SEEKABLE
    ret = isSeekable() ? setDataAtImpl(data, offset) : (offset == 0 ? setDataImpl(data) : -ENOTSUP);
#else
    ret = (offset == 0) ? setDataImpl(data) : -ENOTSUP;
#endif
#ifdef CONFIG_DAWN_IO_HAS_STATS
    if (ret == OK)
      {
        if (offset == 0)
          {
            stats.write_count++;
          }
      }
    else if (ret != -ENOTSUP)
      {
        stats.error_count++;
      }
#endif
    return ret;
  }

  /**
   * @brief Get file descriptor for notifications.
   *
   * Returns an fd for use with select/poll when descriptor changes.
   *
   * @return Valid file descriptor (>=0) on success, -ENOTSUP if not supported.
   */

  virtual int getFd() const
  {
    return -ENOTSUP;
  };

  /**
   * @brief Get data size in bytes.
   *
   * Returns the size of a single data item (without timestamp if present).
   *
   * @return Size in bytes of one data item.
   */

  virtual size_t getDataSize() const = 0;

  /**
   * @brief Get data vector dimension.
   *
   * Returns the number of elements in a data item (e.g., 3 for XYZ
   * accelerometer).
   *
   * @return Number of data elements per item.
   */

  virtual size_t getDataDim() const = 0;

  /**
   * @brief Check if IO supports read operations.
   *
   * @return True if getData() is supported, false otherwise.
   */

  virtual bool isRead() const = 0;

  /**
   * @brief Check if IO supports write operations.
   *
   * @return True if setData() is supported, false otherwise.
   */

  virtual bool isWrite() const = 0;

  /**
   * @brief Check if IO supports notifications.
   *
   * @return True if notification callbacks are supported, false otherwise.
   */

  virtual bool isNotify() const = 0;

  /**
   * @brief Check if IO supports batch operations.
   *
   * @return True if batch data reading/writing is supported, false otherwise.
   */

  virtual bool isBatch() const = 0;

  /**
   * @brief Check if I/O supports timestamp.
   *
   * Returns true if this I/O has timestamp support (IO_FLAGS_TS flag is set).
   *
   * @return True if timestamps are available with data, false otherwise.
   */

  bool isTimestamp() const;

  /**
   * @brief Check if IO supports partial (seekable) access.
   *
   * When true, getData/setData with non-zero offset route to
   * getDataAtImpl/setDataAtImpl. When false, offset > 0 returns -ENOTSUP.
   *
   * @return True if seekable access is supported, false otherwise.
   */

  virtual bool isSeekable() const
  {
    return false;
  }

#ifdef CONFIG_DAWN_IO_NOTIFY
  /**
   * @brief Bind notifier interface for this I/O.
   *
   * Registers a notifier that will receive callbacks when this I/O has data
   * available or events occur.
   *
   * @param n Pointer to IIONotifier interface to bind.
   */

  void bindNotifier(IIONotifier *n);

  /**
   * @brief Set notification callback for this I/O.
   *
   * Registers a callback function that will be invoked when new data is
   * available or an event occurs.
   *
   * @param cb Callback function pointer.
   * @param prio Priority for callback execution.
   * @param priv Private context pointer passed to callback.
   * @return OK on success, negative error code on failure.
   */

  int setNotifier(IIONotifier::notifier_cb_t cb, int prio, void *priv);
#endif

  /**
   * @brief Allocate data buffer for this I/O.
   *
   * Allocates memory for storing data from this I/O.
   *
   * For seekable IOs chunk_size must be non-zero; the allocated buffer is
   * capped to min(chunk_size, getDataSize()) bytes with T=1 (byte) and
   * dtype=UINT8, suitable for partial reads/writes via getData/setData with
   * offset. Passing chunk_size=0 for a seekable IO returns nullptr.
   *
   * For non-seekable IOs chunk_size is ignored and the full IO data size is
   * allocated as usual.
   *
   * @param batch Number of data items to allocate space for (1 for single,
   *   >1 for batch).
   * @param chunk_size Maximum chunk size in bytes for seekable IOs (ignored
   *   for non-seekable IOs; 0 is invalid for seekable IOs).
   * @return Pointer to allocated io_ddata_t structure, nullptr on failure.
   */

  io_ddata_t *ddata_alloc(size_t batch, size_t chunk_size = 0);

  /**
   * @brief Get I/O configuration item.
   *
   * Retrieves a configuration parameter for this I/O.
   *
   * @param cfgid Configuration ID (e.g., IO_CFG_DEVNO, IO_CFG_LIMIT_MIN).
   * @param data Pointer to buffer for configuration data.
   * @param len Length of buffer in 32-bit words.
   * @return OK on success, negative error code on failure.
   */

  int getConfig(uint32_t cfgid, uint32_t *data, size_t len)
  {
    return this->getObjConfig(cfgid, data, len);
  };

  /**
   * @brief Set I/O configuration item.
   *
   * Updates a configuration parameter for this I/O.
   *
   * @param cfgid Configuration ID (e.g., IO_CFG_DEVNO, IO_CFG_LIMIT_MIN).
   * @param data Pointer to configuration data.
   * @param len Length of data in 32-bit words.
   * @return OK on success, negative error code on failure.
   */

  int setConfig(uint32_t cfgid, uint32_t *data, size_t len)
  {
    return this->setObjConfig(cfgid, data, len);
  };

  /**
   * @brief Create ConfigID for I/O common configuration.
   *
   * Constructs a configuration ID for I/O-specific configuration items.
   *
   * @param rw Read-write flag (false=read-only, true=read-write).
   * @param dtype Data type (DTYPE_UINT32, DTYPE_CHAR, etc.).
   * @param size Configuration data size in 32-bit words.
   * @param id Configuration identifier (0-31).
   * @return ConfigID value for use in configuration operations.
   */

  constexpr static SObjectCfg::ObjectCfgId cfgIdCmn(bool rw,
                                                    uint8_t dtype,
                                                    uint8_t size,
                                                    uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_ANY, dtype, rw, size, id);
  }

  /**
   * @brief Create ConfigID for device number configuration.
   *
   * @param rw Read-write flag (default: false for read-only).
   * @return ConfigID for IO_CFG_DEVNO (DTYPE_UINT32, size=1).
   */

  constexpr static SObjectCfg::ObjectCfgId cfgIdDevno(bool rw = false)
  {
    return CIOCommon::cfgIdCmn(rw, SObjectId::DTYPE_UINT32, 1, IO_CFG_DEVNO);
  }

  /**
   * @brief Create ConfigID for minimum I/O limits.
   *
   * @param dtype Limit data type.
   * @param size Limit data size in 32-bit words.
   * @return ConfigID for IO_CFG_LIMIT_MIN.
   */

  constexpr static SObjectCfg::ObjectCfgId cfgIdLimitMin(uint8_t dtype, uint16_t size)
  {
    return CIOCommon::cfgIdCmn(false, dtype, size, IO_CFG_LIMIT_MIN);
  }

  /**
   * @brief Create ConfigID for maximum I/O limits.
   *
   * @param dtype Limit data type.
   * @param size Limit data size in 32-bit words.
   * @return ConfigID for IO_CFG_LIMIT_MAX.
   */

  constexpr static SObjectCfg::ObjectCfgId cfgIdLimitMax(uint8_t dtype, uint16_t size)
  {
    return CIOCommon::cfgIdCmn(false, dtype, size, IO_CFG_LIMIT_MAX);
  }

  /**
   * @brief Create ConfigID for step I/O limits.
   *
   * @param dtype Limit data type.
   * @param size Limit data size in 32-bit words.
   * @return ConfigID for IO_CFG_LIMIT_STEP.
   */

  constexpr static SObjectCfg::ObjectCfgId cfgIdLimitStep(uint8_t dtype, uint16_t size)
  {
    return CIOCommon::cfgIdCmn(false, dtype, size, IO_CFG_LIMIT_STEP);
  }

  /**
   * @brief Create ConfigID for notifier configuration.
   *
   * @param rw Read-write flag (default: false for read-only).
   * @return ConfigID for IO_CFG_NOTIFY (DTYPE_UINT32, size=2).
   */

  constexpr static SObjectCfg::ObjectCfgId cfgIdNotify(bool rw = false)
  {
    return CIOCommon::cfgIdCmn(rw, SObjectId::DTYPE_UINT32, 3, IO_CFG_NOTIFY);
  }

#ifdef CONFIG_DAWN_IO_NOTIFY
  /**
   * @brief Get configured notifier type.
   *
   * @return Notifier type (IO_NOTIFY_POLL or IO_NOTIFY_STREAM).
   */

  uint8_t getNotifyType() const
  {
    return notifyType;
  }

  /**
   * @brief Get configured notifier thread priority.
   *
   * @return Priority value (0 = default).
   */

  int getNotifyPrio() const
  {
    return notifyPrio;
  }

  /**
   * @brief Get configured notifier batch count.
   *
   * @return Batch count (1 = single sample, >1 = multi-sample).
   */

  size_t getNotifyBatch() const
  {
    return notifyBatch;
  }
#endif

protected:
#ifdef CONFIG_DAWN_IO_NOTIFY
  /** @brief Notifier interface pointer, bound during initialization. */

  IIONotifier *notifier;
#endif

  /**
   * @brief Get data implementation (override in derived classes).
   *
   * @param data Reference to IODataCmn structure to receive data.
   * @param len Number of data items to read.
   * @return OK on success, -ENOTSUP if not supported, other negative on error.
   */

  virtual int getDataImpl(IODataCmn &data, size_t len)
  {
    (void)data;
    (void)len;
    return -ENOTSUP;
  }

  /**
   * @brief Set data implementation (override in derived classes).
   *
   * @param data Reference to IODataCmn structure containing data.
   * @return OK on success, -ENOTSUP if not supported, other negative on error.
   */

  virtual int setDataImpl(IODataCmn &data)
  {
    (void)data;
    return -ENOTSUP;
  }

  /**
   * @brief Get data at byte offset (override in seekable IOs).
   *
   * Called by getData() when offset > 0. Default returns -ENOTSUP.
   * Seekable IOs override this to support partial reads.
   * len must be 1; implementations should return -EINVAL otherwise.
   *
   * @param data Reference to IODataCmn buffer to receive data chunk.
   * @param len Number of items (must be 1 for seekable IOs).
   * @param offset Byte offset into IO data.
   * @return OK on success, -ENOTSUP if not supported, other negative on error.
   */

  virtual int getDataAtImpl(IODataCmn &data, size_t len, size_t offset)
  {
    (void)data;
    (void)len;
    (void)offset;
    return -ENOTSUP;
  }

  /**
   * @brief Set data at byte offset (override in seekable IOs).
   *
   * Called by setData() when offset > 0. Default returns -ENOTSUP.
   * Seekable IOs override this to support partial writes.
   *
   * @param data Reference to IODataCmn buffer containing data chunk.
   * @param offset Byte offset into IO data.
   * @return OK on success, -ENOTSUP if not supported, other negative on error.
   */

  virtual int setDataAtImpl(IODataCmn &data, size_t offset)
  {
    (void)data;
    (void)offset;
    return -ENOTSUP;
  }

#ifdef CONFIG_DAWN_IO_NOTIFY
  /**
   * @brief Dispatch already-available data through the bound notifier.
   *
   * This is for write-driven notifications where the payload is already known,
   * so the notifier does not need to wake through getFd()/poll().
   */

  int notifyData(io_ddata_t *data);
#endif

  /**
   * @brief Get device number for this I/O.
   *
   * @return Device number configured for this I/O.
   */

  int getCmnDevno() const
  {
    return this->devno;
  }

  /**
   * @brief Get minimum I/O limit words.
   */

  const uint32_t *getCmnLimitMin()
  {
    return this->limits.getMin();
  }

  /**
   * @brief Get maximum I/O limit words.
   */

  const uint32_t *getCmnLimitMax()
  {
    return this->limits.getMax();
  }

  /**
   * @brief Get step I/O limit words.
   */

  const uint32_t *getCmnLimitStep()
  {
    return this->limits.getStep();
  }

  /**
   * @brief Get number of 32-bit words in I/O limit arrays.
   */

  size_t getCmnLimitWords()
  {
    return this->limits.getWords();
  }

  /**
   * @brief Get common IO limits container.
   */

  const CIOLimits &getCmnLimits() const
  {
    return this->limits;
  }

  /**
   * @brief Get offset of configuration item in descriptor.
   *
   * @param cfg Pointer to configuration item.
   * @return Byte offset of configuration item.
   */

  size_t cfgCmnOffset(const SObjectCfg::SObjectCfgItem *cfg);

  /**
   * @brief Get current timestamp.
   *
   * @return Current system timestamp in microseconds.
   */

  uint64_t getTimestamp();

private:
  int devno;          ///< Device number for underlying hardware device
  CIOLimits limits;   ///< Common IO limits container
#ifdef CONFIG_DAWN_IO_NOTIFY
  uint8_t notifyType; ///< Configured notifier type (IO_NOTIFY_POLL or IO_NOTIFY_STREAM)
  int notifyPrio;     ///< Configured notifier thread priority (0 = default)
  size_t notifyBatch; ///< Configured notifier batch count (1 = single sample)
#endif
#ifdef CONFIG_DAWN_IO_HAS_STATS
  IOStats stats;      ///< Runtime statistics
#endif

  /**
   * @brief Configure IO from descriptor.
   *
   * Parses descriptor and applies IO configuration.
   *
   * @param desc Descriptor object containing configuration.
   * @return OK on success, negative error code on failure.
   */

  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
