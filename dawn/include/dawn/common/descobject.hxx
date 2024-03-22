// dawn/include/dawn/common/descobject.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/objectcfg.hxx"
#include "dawn/common/objectid.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Descriptor wrapper for individual object configuration.
 *
 * Provides convenient access to object configuration data stored in a
 * descriptor binary.
 */

class CDescObject
{
public:
  /**
   * @brief Construct CDescObject from raw descriptor pointer.
   *
   * Wraps raw 32-bit pointer to descriptor data.
   *
   * @param cfg Reference to SObjectCfgData in descriptor.
   */

  explicit CDescObject(SObjectCfg::SObjectCfgData &cfg)
    : cfgobj(cfg)
    , size(SObjectCfg::getSizeBytes(cfgobj))
  {
  }

  /**
   * @brief Construct CDescObject from raw descriptor pointer.
   *
   * Wraps raw 32-bit pointer to descriptor data.
   *
   * @param cfg Pointer to descriptor data (32-bit word pointer).
   */

  explicit CDescObject(uint32_t *cfg)
    : cfgobj(*(reinterpret_cast<SObjectCfg::SObjectCfgData *>(cfg)))
    , size(SObjectCfg::getSizeBytes(cfgobj))
  {
  }

  /** @brief Destructor. */

  ~CDescObject() = default;

  /**
   * @brief Get object identifier as union structure.
   *
   * Returns the complete object ID with access to bit fields (type, class,
   * dtype, flags, instance).
   *
   * @return Reference to UObjectId structure containing object ID fields.
   */

  SObjectId::UObjectId &getObjectId() const;

  /**
   * @brief Get object identifier as raw 32-bit value.
   *
   * Returns complete object ID as single 32-bit value.
   *
   * @return ObjectId (uint32_t) containing full object metadata.
   */

  SObjectId::ObjectId getObjectIdV() const;

  /**
   * @brief Get object class field.
   *
   * Extracts class identifier from object ID.
   *
   * @return Object class (0-511).
   */

  uint16_t getObjectCls() const;

  /**
   * @brief Get object type field.
   *
   * Extracts type from object ID.
   *
   * @return Object type (OBJTYPE_ANY, OBJTYPE_IO, OBJTYPE_PROTO, etc.).
   */

  uint8_t getObjectType() const;

  /**
   * @brief Get data type field.
   *
   * Extracts data type from object ID.
   *
   * @return Data type (DTYPE_UINT32, DTYPE_FLOAT, etc.).
   */

  uint8_t getObjectDtype() const;

  /**
   * @brief Get underlying descriptor data structure.
   *
   * Returns pointer to raw SObjectCfgData in descriptor.
   *
   * @return Pointer to SObjectCfgData (or nullptr if invalid).
   */

  SObjectCfg::SObjectCfgData *getCfg() const;

  /**
   * @brief Get number of configuration items for this object.
   *
   * Returns count of configuration items stored in descriptor for this object.
   *
   * @return Number of configuration items (0-32).
   */

  size_t getSize() const;

  /**
   * @brief Get total size in bytes for this object definition.
   *
   * Returns the complete size of object definition in descriptor, including
   * object metadata and all configuration items/data.
   *
   * @return Total object size in bytes.
   */

  size_t getSizeBytes() const;

  /**
   * @brief Get configuration item at specified offset.
   *
   * Retrieves configuration item at given offset within object.
   *
   * @param offset Offset in 32-bit words from object start.
   * @return Pointer to SObjectCfgItem at offset, nullptr if invalid.
   */

  SObjectCfg::SObjectCfgItem *objectCfgItemAtOffset(size_t offset) const;

  /**
   * @brief Get config item at current offset and advance past it.
   *
   * Returns the item at offset, then atomically advances offset by
   * 1 (cfgid header word) plus item->cfgid.s.size (data words), so
   * offset points to the next config item on return. Use item->data[]
   * to access the item's payload without a separate pointer lookup.
   *
   * @param offset Current word offset; updated in-place to point past
   *               this item on return.
   * @return Pointer to SObjectCfgItem at entry offset.
   */

  SObjectCfg::SObjectCfgItem *objectCfgItemNext(size_t &offset) const;

  /**
   * @brief Get configuration item with specified ConfigID.
   *
   * Searches configuration items for matching ConfigID and returns it.
   *
   * @param id ConfigID to search for.
   * @return Pointer to matching SObjectCfgItem, nullptr if not found.
   */

  SObjectCfg::SObjectCfgItem *objectCfgItemId(SObjectCfg::ObjectCfgId id) const;

  /**
   * @brief Get 32-bit word at specified offset.
   *
   * Convenient accessor for reading a single 32-bit word from object's
   * descriptor data.
   *
   * @param offset Offset in 32-bit words from object start.
   * @return 32-bit value at offset (host endian).
   */

  uint32_t getAtOffset(size_t offset) const;

private:
  /** @brief Reference to descriptor's SObjectCfgData. */

  SObjectCfg::SObjectCfgData &cfgobj;

  /** @brief Cached total size of this object in 32-bit words. */

  size_t size;
};
} // Namespace dawn
