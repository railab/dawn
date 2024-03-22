// dawn/include/dawn/common/objectcfg.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/objectid.hxx"
#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/porting/fixedmath.hxx"

#include <cstring>
#include <cstdio>

namespace dawn
{
/**
 * @brief Configuration management for Dawn objects.
 *
 * Encodes and manages optional configuration data attached to objects.
 *
 * ConfigID is a 32-bit packed field:
 *
 *   - id[0:4],
 *   - size[5:14] (32-bit words),
 *   - rw[15],
 *   - dtype[16:19],
 *   - class[21:29],
 *   - type[30:31].
 *
 * These fields identify a config item and describe its storage and access
 * semantics.
 */

class SObjectCfg
{
public:
  /**
   * @brief Configuration ID field constants and bit shift positions.
   *
   * Used with getter functions to extract fields from ConfigID.
   */

  constexpr static size_t ID_MAX = 0b11111;
  constexpr static size_t ID_SHIFT = 0;
  constexpr static size_t _SIZE_MAX = 0b1111111111;
  constexpr static size_t SIZE_SHIFT = 5;
  constexpr static size_t RW_MAX = 0b1;
  constexpr static size_t RW_SHIFT = 15;
  constexpr static size_t DTYPE_MAX = 0b1111;
  constexpr static size_t DTYPE_SHIFT = 16;
  constexpr static size_t CLS_MAX = 0b111111111;
  constexpr static size_t CLS_SHIFT = 21;
  constexpr static size_t TYPE_MAX = 0b11;
  constexpr static size_t TYPE_SHIFT = 30;

  /** @brief ConfigID type - single 32-bit value. */

  typedef uint32_t ObjectCfgId;

  /**
   * @brief Configuration data element - single 32-bit word.
   *
   * All configuration data is stored as arrays of 32-bit words, regardless of
   * actual data type.
   */

  typedef uint32_t ObjectCfgData_t;

  /**
   * @brief 32-bit encoded configuration identifier (union with bit field).
   *
   * Uniquely identifies a single configuration item within an object's
   * configuration set.
   */

  union
  {
    /** @brief Raw 32-bit ConfigID value (for storage, comparison). */

    ObjectCfgId v;

    /** @brief Bit-field structure for named member access. */

    struct
    {
      /**
       * @brief Configuration identifier (bits 0-4, max 31).
       *
       * Unique ID within object's configuration set.
       */

      uint32_t id : 5;

      /**
       * @brief Configuration data size in 32-bit words (bits 5-14, max 1023).
       *
       * Total size of config data.
       */

      uint32_t size : 10;

      /**
       * @brief Read-write access flag (bit 15).
       *
       * 0 = read-only (immutable, from descriptor)
       * 1 = read-write (can be modified at runtime) Framework enforces flag in
       *                 setObjConfig()/getObjConfig().
       */

      uint32_t rw : 1;

      /**
       * @brief Configuration data type (bits 16-19).
       *
       * Matches EObjectDataType enumeration.
       */

      uint32_t dtype : 4;

      /**
       * @brief Extension flag (bit 20).
       *
       * Reserved for future 64-bit ConfigID extension.
       */

      uint32_t ext : 1;

      /**
       * @brief Object class (bits 21-29, max 511).
       *
       * Must match parent object's class field.
       */

      uint32_t cls : 9;

      /**
       * @brief Object type (bits 30-31).
       *
       * Must match parent object's type field.
       */

      uint32_t type : 2;
    } s;
  } typedef UObjectCfgId;

  /**
   * @brief Single configuration item within object.
   *
   * Contains a configuration ID header followed by configuration data.
   */

  struct
  {
    /** @brief Configuration ID header (type, class, id, size, rw, dtype). */

    UObjectCfgId cfgid;

    /**
     * @brief Configuration data array (flexible, size from cfgid.s.size).
     *
     * Stored as 32-bit words; interpret based on cfgid.s.dtype.
     */

    ObjectCfgData_t data[];
  } typedef SObjectCfgItem;

  /**
   * @brief Object configuration data container.
   *
   * Represents complete configuration for a single object in descriptor.
   */

  struct
  {
    /** @brief Object identifier (type, class, dtype, instance). */

    SObjectId::UObjectId objid;

    /** @brief Number of configuration items for this object (0-32). */

    uint32_t size;

    /**
     * @brief First configuration item (array continues in memory).
     *
     * Remaining items (size-1) follow sequentially with variable sizes.
     */

    SObjectCfgItem items;
  } typedef SObjectCfgData;

  /**
   * @brief Construct 32-bit ConfigID from component fields.
   *
   * Packs individual configuration metadata into a single 32-bit ConfigID.
   *
   * @param type Object type (0-3): OBJTYPE_ANY, OBJTYPE_IO, etc.
   * @param cls Object class (0-511): IOCLS_ADC, IOCLS_GPIO, etc.
   * @param dtype Data type (0-15): DTYPE_UINT32, DTYPE_FLOAT, etc.
   * @param rw Read-write flag: false=read-only, true=read-write.
   * @param size Configuration data size in 32-bit words (0-1023).
   * @param id Configuration identifier (0-31).
   * @return Constructed 32-bit ConfigID value.
   */

  constexpr static ObjectCfgId objectCfg(uint8_t type,
                                         uint16_t cls,
                                         uint8_t dtype,
                                         bool rw,
                                         uint16_t size,
                                         uint8_t id)
  {
    DEBUGASSERT(type <= TYPE_MAX);
    DEBUGASSERT(cls <= CLS_MAX);
    DEBUGASSERT(dtype <= DTYPE_MAX);
    DEBUGASSERT(size <= _SIZE_MAX);
    DEBUGASSERT(id <= ID_MAX);

    return (((type & TYPE_MAX) << TYPE_SHIFT) | ((cls & CLS_MAX) << CLS_SHIFT) |
            ((dtype & DTYPE_MAX) << DTYPE_SHIFT) | ((id & ID_MAX) << ID_SHIFT) |
            ((size & _SIZE_MAX) << SIZE_SHIFT) | ((rw & RW_MAX) << RW_SHIFT));
  }

  /**
   * @brief Extract configuration identifier from ConfigID.
   *
   * @param objcfg ConfigID value.
   * @return Configuration identifier (0-31).
   */

  constexpr static uint8_t objectCfgGetId(const ObjectCfgId objcfg)
  {
    return ((objcfg >> ID_SHIFT) & ID_MAX);
  }

  /**
   * @brief Extract configuration data size from ConfigID.
   *
   * @param objcfg ConfigID value.
   * @return Configuration data size in 32-bit words (0-1023).
   */

  constexpr static uint16_t objectCfgGetSize(const ObjectCfgId objcfg)
  {
    return ((objcfg >> SIZE_SHIFT) & _SIZE_MAX);
  }

  /**
   * @brief Extract read-write access flag from ConfigID.
   *
   * @param objcfg ConfigID value.
   * @return true if read-write (can be modified), false if read-only.
   */

  constexpr static bool objectCfgGetRw(const ObjectCfgId objcfg)
  {
    return ((objcfg >> RW_SHIFT) & RW_MAX);
  }

  /**
   * @brief Extract data type from ConfigID.
   *
   * @param objcfg ConfigID value.
   * @return Data type (EObjectDataType value, 0-15).
   */

  constexpr static uint8_t objectCfgGetDtype(const ObjectCfgId objcfg)
  {
    return ((objcfg >> DTYPE_SHIFT) & DTYPE_MAX);
  }

  /**
   * @brief Extract object class from ConfigID.
   *
   * @param objcfg ConfigID value.
   * @return Object class (0-511).
   */

  constexpr static uint16_t objectCfgGetCls(const ObjectCfgId objcfg)
  {
    return ((objcfg >> CLS_SHIFT) & CLS_MAX);
  }

  /**
   * @brief Extract object type from ConfigID.
   *
   * @param objcfg ConfigID value.
   * @return Object type (EObjectIdType value, 0-3).
   */

  constexpr static uint8_t objectCfgGetType(const ObjectCfgId objcfg)
  {
    return ((objcfg >> TYPE_SHIFT) & TYPE_MAX);
  }

  /** @brief Helper conversion functions: data types to cfg data type. */

  /**
   * @brief Convert uint32_t to ObjectCfgData_t.
   *
   * @param x uint32_t value.
   * @return Converted ObjectCfgData_t.
   */

  static ObjectCfgData_t u32ToCfg(uint32_t x)
  {
    return x;
  }

  /**
   * @brief Convert int32_t to ObjectCfgData_t.
   *
   * @param x int32_t value.
   * @return Converted ObjectCfgData_t.
   */

  static ObjectCfgData_t i32ToCfg(int32_t x)
  {
    ObjectCfgData_t y;

    std::memcpy(&y, &x, sizeof(y));
    return y;
  }

  /**
   * @brief Convert float to ObjectCfgData_t.
   *
   * @param x float value.
   * @return Converted ObjectCfgData_t.
   */

  static ObjectCfgData_t fToCfg(float x)
  {
    ObjectCfgData_t y;

    std::memcpy(&y, &x, sizeof(y));
    return y;
  }

  /**
   * @brief Convert b16_t to uint32_t.
   *
   * @param x b16_t value.
   * @return Converted uint32_t.
   */

  static uint32_t b16ToCfg(b16_t x)
  {
    ObjectCfgData_t y;

    std::memcpy(&y, &x, sizeof(y));
    return y;
  }

  /**
   * @brief Convert float to b16_t and then to ObjectCfgData_t.
   *
   * @param x float value.
   * @return Converted ObjectCfgData_t.
   */

  static ObjectCfgData_t fToB16ToCfg(float x)
  {
    b16_t y = ftob16(x);
    return b16ToCfg(y);
  }

  /**
   * @brief Convert ObjectCfgData_t to uint32_t.
   *
   * @param x ObjectCfgData_t value.
   * @return Converted uint32_t.
   */

  static uint32_t cfgToU32(ObjectCfgData_t x)
  {
    return x;
  }

  /**
   * @brief Convert ObjectCfgData_t to int32_t.
   *
   * @param x ObjectCfgData_t value.
   * @return Converted int32_t.
   */

  static int32_t cfgtoi32(ObjectCfgData_t x)
  {
    int32_t y;

    std::memcpy(&y, &x, sizeof(y));
    return y;
  }

  /**
   * @brief Convert ObjectCfgData_t to float.
   *
   * @param x ObjectCfgData_t value.
   * @return Converted float.
   */

  static float cfgToF(ObjectCfgData_t x)
  {
    float y;

    std::memcpy(&y, &x, sizeof(y));
    return y;
  }

  /**
   * @brief Convert ObjectCfgData_t to b16_t.
   *
   * @param x ObjectCfgData_t value.
   * @return Converted b16_t.
   */

  static b16_t cfgToB16(ObjectCfgData_t x)
  {
    b16_t y;

    std::memcpy(&y, &x, sizeof(y));
    return y;
  }

  /**
   * @brief Get item from offset.
   *
   * @param cfg Object configuration data.
   * @param offset Offset in 32-bit words.
   * @return Pointer to configuration item.
   *
   * @note Returns non-const pointer from a const-ref parameter.
   * The underlying descriptor data resides in mutable memory
   * (allocated during descriptor construction); the const qualifier
   * on cfg is for API clarity, not physical constness of the data.
   */

  static SObjectCfgItem *objectAtOffset(const SObjectCfgData &cfg, size_t offset)
  {
    return reinterpret_cast<SObjectCfgItem *>(
      reinterpret_cast<uint32_t *>(const_cast<SObjectCfgData *>(&cfg)) + offset);
  }

  /**
   * @brief Get item from configuration ID.
   *
   * @param cfg Object configuration data.
   * @param id Configuration ID.
   * @return Pointer to configuration item, nullptr if not found.
   */

  static SObjectCfgItem *objectCfgFromCfgId(const SObjectCfgData &cfg, ObjectCfgId id);

  /**
   * @brief Get total size in bytes for this object definition.
   *
   * Returns the complete size of object definition in descriptor, including
   * object metadata and all configuration items/data.
   */

  static size_t getSizeBytes(const SObjectCfgData &cfg);
};
} // Namespace dawn
