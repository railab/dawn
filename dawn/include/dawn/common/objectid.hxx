// dawn/include/dawn/common/objectid.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>
#include <cstdlib>

#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief 32-bit encoded Object ID with type, class, data type, and instance.
 *
 * Fundamental identifier for all Dawn objects.
 */

struct SObjectId
{
  /** @brief Bit field constants and shift positions for ObjectID extraction. */

  constexpr static size_t PRIV_MAX = 0b11111111111111;
  constexpr static size_t PRIV_SHIFT = 0;
  constexpr static size_t FLAGS_MAX = 0b11;
  constexpr static size_t FLAGS_SHIFT = 14;
  constexpr static size_t DTYPE_MAX = 0b1111;
  constexpr static size_t DTYPE_SHIFT = 16;
  constexpr static size_t EXT_MAX = 0b1;
  constexpr static size_t EXT_SHIFT = 20;
  constexpr static size_t CLS_MAX = 0b111111111;
  constexpr static size_t CLS_SHIFT = 21;
  constexpr static size_t CLS_MASK = (CLS_MAX << CLS_SHIFT);
  constexpr static size_t TYPE_MAX = 0b11;
  constexpr static size_t TYPE_SHIFT = 30;
  constexpr static size_t TYPE_MASK = (TYPE_MAX << TYPE_SHIFT);
  constexpr static size_t fCLS_ANY = 0;

  /** @brief ObjectID type - single 32-bit value. */

  typedef uint32_t ObjectId;

  /**
   * @brief Extended ObjectID type - future 64-bit support.
   *
   * Not currently supported.
   */

  typedef uint64_t ObjectIdExt;

  /**
   * @brief Data types supported by Dawn framework.
   *
   * Defines all supported data types for object values and configurations.
   */

  enum
  {
    /**
     * @brief Wildcard data type (matches any actual type).
     *
     * Used in template configuration items.
     */

    DTYPE_ANY = 0,

    /** @brief Boolean data type (stored in 32-bit container). */

    DTYPE_BOOL = 1,

    /** @brief Signed 8-bit integer (-128 to 127). */

    DTYPE_INT8 = 2,

    /** @brief Unsigned 8-bit integer (0 to 255). */

    DTYPE_UINT8 = 3,

    /** @brief Signed 16-bit integer (-32768 to 32767). */

    DTYPE_INT16 = 4,

    /** @brief Unsigned 16-bit integer (0 to 65535). */

    DTYPE_UINT16 = 5,

    /** @brief Signed 32-bit integer (-2147483648 to 2147483647). */

    DTYPE_INT32 = 6,

    /** @brief Unsigned 32-bit integer (0 to 4294967295). */

    DTYPE_UINT32 = 7,

    /** @brief Signed 64-bit integer. */

    DTYPE_INT64 = 8,

    /** @brief Unsigned 64-bit integer. */

    DTYPE_UINT64 = 9,

    /**
     * @brief IEEE 754 single-precision floating point (32-bit).
     *
     * Range: ~±10^-38 to ±10^38, 6-7 decimal digits precision.
     */

    DTYPE_FLOAT = 10,

    /**
     * @brief IEEE 754 double-precision floating point (64-bit).
     *
     * Range: ~±10^-308 to ±10^308, 15-17 decimal digits precision.
     */

    DTYPE_DOUBLE = 11,

    /**
     * @brief Signed 16.16 fixed-point (32-bit).
     *
     * Range: -32768 to 32767 with 1/65536 precision.
     */

    DTYPE_B16 = 12,

    /** @brief Unsigned 16.16 fixed-point (32-bit). */

    DTYPE_UB16 = 13,

    /**
     * @brief Character/string type (null-terminated, 4-byte aligned).
     *
     * Data storage:
     * - Must be null-terminated.
     * - Stored in 32-bit words and padded to a 4-byte boundary.
     * - Example: "hello" (5 chars) needs 2 words (8 bytes).
     * - Size field indicates total 32-bit words (including padding).
     */

    DTYPE_CHAR = 14,

    /**
     * @brief Opaque block/byte-stream data type.
     *
     * Intended for long or binary payloads accessed as 1-byte units.
     * IOs using this type should support seekable access.
     */

    DTYPE_BLOCK = 15,

    /** @brief Sentinel value (last valid type + 1). */

    DTYPE_LAST = 16,
  } typedef EObjectDataType;

  static_assert(DTYPE_LAST - 1 <= DTYPE_MAX);

  /**
   * @brief Object types in the Dawn framework.
   *
   * Defines the categories of objects managed by the framework.
   */

  enum
  {
    /**
     * @brief Wildcard/metadata object type.
     *
     * Used for descriptor metadata (version, device string).
     */

    OBJTYPE_ANY = 0,

    /**
     * @brief Input/Output object type.
     *
     * Managed by: CIOHandler Purpose: Physical device I/O and virtual I/O.
     */

    OBJTYPE_IO = 1,

    /**
     * @brief Protocol object type.
     *
     * Managed by: CProtoHandler Purpose: Device-to-external communication
     * (host, cloud, peers).
     */

    OBJTYPE_PROTO = 2,

    /**
     * @brief Program/algorithm object type.
     *
     * Managed by: CProgHandler Purpose: Edge processing and data
     * transformation.
     */

    OBJTYPE_PROG = 3,

    /** @brief Sentinel value (last valid type + 1). */

    OBJTYPE_LAST = 4,
  } typedef EObjectIdType;

  static_assert(OBJTYPE_LAST - 1 <= TYPE_MAX);

  /**
   * @brief 32-bit encoded object identifier (union with bit field).
   *
   * Uniquely identifies a single Dawn object using bit-packed representation.
   */

  union
  {
    /** @brief Raw 32-bit ObjectID value (for comparison, hashing, storage). */

    ObjectId v;

    /** @brief Bit-field structure for named member access. */

    struct
    {
      /**
       * @brief Instance/private data field (bits 0-13, max 16383).
       *
       * Typically instance number within a type+class combination.
       */

      uint32_t priv : 14;

      /**
       * @brief Type-specific flags (bits 14-15, max 3).
       *
       * Interpretation depends on object type and class.
       */

      uint32_t flags : 2;

      /**
       * @brief Data type field (bits 16-19, see EObjectDataType).
       *
       * Specifies data format: DTYPE_INT16, DTYPE_FLOAT, DTYPE_CHAR, etc.
       */

      uint32_t dtype : 4;

      /**
       * @brief Extension flag (bit 20).
       *
       * Reserved for future 64-bit ObjectID extension.
       */

      uint32_t ext : 1;

      /**
       * @brief Object class field (bits 21-29, max 511).
       *
       * Type-specific class identifier determining concrete object type.
       */

      uint32_t cls : 9;

      /**
       * @brief Object type field (bits 30-31, see EObjectIdType).
       *
       * Determines handler responsibility (IO, PROG, PROTO, or ANY).
       */

      uint32_t type : 2;
    } s;
  } typedef UObjectId;

  /**
   * @brief Construct 32-bit ObjectID from component fields.
   *
   * Packs individual fields into a single 32-bit ObjectID value.
   *
   * @param type Object type (0-3): OBJTYPE_ANY, OBJTYPE_IO, etc.
   * @param cls Object class (0-511): IOCLS_ADC, IOCLS_GPIO, etc.
   * @param dtype Data type (0-15): DTYPE_UINT32, DTYPE_FLOAT, etc.
   * @param flags Type-specific flags (0-3): IO_FLAGS_TS, etc.
   * @param priv Instance/private field (0-16383): instance number.
   * @return Constructed 32-bit ObjectID value.
   */

  constexpr static ObjectId objectId(uint8_t type,
                                     uint16_t cls,
                                     uint8_t dtype,
                                     uint8_t flags,
                                     uint16_t priv)
  {
    DEBUGASSERT(type <= TYPE_MAX);
    DEBUGASSERT(cls <= CLS_MAX);
    DEBUGASSERT(dtype <= DTYPE_MAX);
    DEBUGASSERT(priv <= PRIV_MAX);

    return (((type & TYPE_MAX) << TYPE_SHIFT) | ((cls & CLS_MAX) << CLS_SHIFT) |
            ((dtype & DTYPE_MAX) << DTYPE_SHIFT) | ((flags & FLAGS_MAX) << FLAGS_SHIFT) |
            ((priv & PRIV_MAX) << PRIV_SHIFT));
  }

  /**
   * @brief Extract instance/private field from ObjectID.
   *
   * @param objid ObjectID value.
   * @return Instance ID (0-16383).
   */

  constexpr static uint16_t objectIdGetId(const ObjectId objid)
  {
    return ((objid >> PRIV_SHIFT) & PRIV_MAX);
  };

  /**
   * @brief Extract type-specific flags from ObjectID.
   *
   * @param objid ObjectID value.
   * @return Flags value (0-3).
   */

  constexpr static uint8_t objectIdGetFlags(const ObjectId objid)
  {
    return ((objid >> FLAGS_SHIFT) & FLAGS_MAX);
  };

  /**
   * @brief Extract data type field from ObjectID.
   *
   * @param objid ObjectID value.
   * @return Data type (EObjectDataType value, 0-15).
   */

  constexpr static uint8_t objectIdGetDtype(const ObjectId objid)
  {
    return ((objid >> DTYPE_SHIFT) & DTYPE_MAX);
  };

  /**
   * @brief Extract object class from ObjectID.
   *
   * @param objid ObjectID value.
   * @return Object class (0-511).
   */

  constexpr static uint16_t objectIdGetCls(const ObjectId objid)
  {
    return ((objid >> CLS_SHIFT) & CLS_MAX);
  };

  /**
   * @brief Extract object type from ObjectID.
   *
   * @param objid ObjectID value.
   * @return Object type (EObjectIdType value, 0-3).
   */

  constexpr static uint8_t objectIdGetType(const ObjectId objid)
  {
    return ((objid >> TYPE_SHIFT) & TYPE_MAX);
  };

  /**
   * @brief Extract "type-agnostic" ObjectID (zero instance field).
   *
   * Returns ObjectID with instance/priv field zeroed.
   *
   * @param objid ObjectID value.
   * @return ObjectID with instance field zeroed (only type+class+dtype+flags).
   */

  constexpr static uint32_t objectIdGetNoId(const ObjectId objid)
  {
    return objid & (~PRIV_MAX);
  };

  /**
   * @brief Check if object is I/O type (union structure version).
   *
   * @param objid Reference to UObjectId structure.
   * @return True if objid.s.type == OBJTYPE_IO.
   */

  constexpr static bool objectIsIO(const UObjectId &objid)
  {
    return (objid.s.type == OBJTYPE_IO);
  };

  /**
   * @brief Check if object is Protocol type (raw 32-bit version).
   *
   * @param objid Reference to UObjectId structure.
   * @return True if objid.s.type == OBJTYPE_PROTO.
   */

  constexpr static bool objectIsProto(const UObjectId &objid)
  {
    return (objid.s.type == OBJTYPE_PROTO);
  };

  /**
   * @brief Check if object is Program type (raw 32-bit version).
   *
   * @param objid Reference to UObjectId structure.
   * @return True if objid.s.type == OBJTYPE_PROG.
   */

  constexpr static bool objectIsProg(const UObjectId &objid)
  {
    return (objid.s.type == OBJTYPE_PROG);
  };

  /**
   * @brief Check if object is I/O type (union structure version).
   *
   * @param objid ObjectID raw value.
   * @return True if object type is OBJTYPE_IO.
   */

  constexpr static bool objectIsIO(const ObjectId objid)
  {
    return objectIdGetType(objid) == OBJTYPE_IO;
  };

  /**
   * @brief Check if object is Protocol type (raw 32-bit version).
   *
   * @param objid ObjectID raw value.
   * @return True if object type is OBJTYPE_PROTO.
   */

  constexpr static bool objectIsProto(const ObjectId objid)
  {
    return objectIdGetType(objid) == OBJTYPE_PROTO;
  };

  /**
   * @brief Check if object is Program type (raw 32-bit version).
   *
   * @param objid ObjectID raw value.
   * @return True if object type is OBJTYPE_PROG.
   */

  constexpr static bool objectIsProg(const ObjectId objid)
  {
    return objectIdGetType(objid) == OBJTYPE_PROG;
  };

  /**
   * @brief Extract type and class mask from ObjectID.
   *
   * Returns ObjectID with instance and flags fields zeroed.
   *
   * @param objid ObjectID raw value.
   * @return ObjectID with instance/priv and flags fields zeroed.
   */

  constexpr static ObjectId objectMask(const ObjectId objid)
  {
    return (objid & TYPE_MASK) | (objid & CLS_MASK);
  };

  /**
   * @brief Get byte size for a specific data type.
   *
   * Returns the size in bytes of values for the given data type.
   *
   * @param dtype Data type (EObjectDataType value).
   * @return Size in bytes: 1, 2, 4, or 8 depending on type.
   */

  static int getDtypeSize_(const EObjectDataType dtype);

  /**
   * @brief Check if a data type is enabled in build configuration.
   *
   * DTYPE_ANY is always considered supported.
   *
   * @param dtype Data type (EObjectDataType value).
   * @return True if enabled, false otherwise.
   */

  static bool isDtypeSupported(uint8_t dtype);

  /**
   * @brief Convert data type to human-readable string.
   *
   * Returns string representation of data type enum value.
   *
   * @param dtype Data type (EObjectDataType value).
   * @return String like "int32", "float", "bool", etc. Returns "???" for
   *         unknown types.
   */

  static const char *dtypeToString(uint8_t dtype);
};
} // Namespace dawn
