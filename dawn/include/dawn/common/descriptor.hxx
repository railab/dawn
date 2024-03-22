// dawn/include/dawn/common/descriptor.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <functional>
#include <vector>

#include "dawn/common/descobject.hxx"
#include "dawn/common/handler.hxx"
#include "dawn/common/object.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/** @brief Binary device descriptor manager. */

class CDescriptor
{
public:
  /** @brief Header magic number (0x0d0a0302). */

  constexpr static uint32_t DAWN_DESCRIPTOR_HDR = 0x0d0a0302;

  /** @brief Footer magic number (0x02030a0d). */

  constexpr static uint32_t DAWN_DESCRIPTOR_FOOT = 0x02030a0d;

  /** @brief Binary descriptor header structure. */

  struct
  {
    uint32_t magic;          // Header magic (0x0d0a0302)
    uint32_t size : 16;      // Number of objects
    uint32_t _reserved : 16; // Reserved
  } typedef SDescriptorBinHdr;

  /** @brief Binary descriptor footer structure. */

  struct
  {
    uint32_t magic; // Footer magic (0x02030a0d)
    uint32_t sum;   // CRC32 checksum
  } typedef SDescriptorBinFtr;

  /** @brief Complete binary descriptor container. */

  struct
  {
    SDescriptorBinHdr hdr;              // Header (8 bytes)
    SObjectCfg::SObjectCfgData objects; // Objects
  } typedef SDescriptorBin;

  /** @brief Descriptor metadata configuration IDs. */

  enum
  {
    DESC_CFG_FIRST = 0,
    DESC_CFG_VERSION = 1,
    DESC_CFG_STRING = 2,
    DESC_CFG_NO_IDLE_QUIT = 3,
    DESC_CFG_LAST = 31
  };

  /**
   * @brief Construct 32-bit ObjectID from component fields.
   *
   * Packs individual fields into a single 32-bit ObjectID value.
   *
   * @param inst Instance number (typically 0).
   * @return Properly formatted ObjectID.
   */

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(SObjectId::OBJTYPE_ANY, 0, SObjectId::OBJTYPE_ANY, 0, inst);
  }

  /**
   * @brief Create ConfigID for metadata.
   *
   * @param rw Read-write flag.
   * @param dtype Data type.
   * @param size Configuration data size in 32-bit words.
   * @param id Configuration identifier.
   * @return Properly formatted ConfigID.
   */

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_ANY, 0, dtype, rw, size, id);
  }

  /**
   * @brief Create ConfigID for firmware version.
   *
   * @param rw Read-write flag (default: false).
   * @return Properly formatted ConfigID for version.
   */

  constexpr static SObjectCfg::ObjectCfgId cfgIdVersion(bool rw = false)
  {
    return CDescriptor::cfgId(rw, SObjectId::DTYPE_UINT32, 1, DESC_CFG_VERSION);
  }

  /**
   * @brief Create ConfigID for device string.
   *
   * @param size Configuration data size in 32-bit words.
   * @param rw Read-write flag (default: false).
   * @return Properly formatted ConfigID for string.
   */

  constexpr static SObjectCfg::ObjectCfgId cfgIdString(uint16_t size, bool rw = false)
  {
    return CDescriptor::cfgId(rw, SObjectId::DTYPE_CHAR, size, DESC_CFG_STRING);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdNoIdleQuit()
  {
    return CDescriptor::cfgId(false, SObjectId::DTYPE_UINT8, 1, DESC_CFG_NO_IDLE_QUIT);
  }

  /** @brief Constructor - initialize empty descriptor. */

  CDescriptor()
    : bindesc(nullptr)
    , bindescLen(0)
    , noIdleQuit(false)
  {
  }

  /** @brief Return true if the descriptor requests no idle quit. */

  bool getNoIdleQuit() const
  {
    return noIdleQuit;
  }

  /** @brief Destructor - release loaded descriptor resources. */

  ~CDescriptor();

  /**
   * @brief Load binary descriptor from memory.
   *
   * @param bin Pointer to binary descriptor data (32-bit aligned).
   * @param len Length in 32-bit words.
   * @param force_valid Recalculate and fill descriptor CRC if true.
   * @param dump Dump descriptor contents if true (default false).
   * @return OK on success, negative error code on failure.
   */

  int loadBin(uint32_t *bin, size_t len, bool force_valid = false, bool dump = false);

  /**
   * @brief Clear currently loaded descriptor state.
   *
   * Releases cached descriptor object wrappers and clears binary pointers.
   */

  void reset();

  /**
   * @brief Get loaded descriptor binary structure.
   *
   * @return Pointer to SDescriptorBin, nullptr if not loaded.
   */

  SDescriptorBin *getBin();

  /**
   * @brief Get loaded descriptor size in bytes.
   *
   * @return SDescriptorBin length in bytes.
   */

  size_t getBinLen();

  /**
   * @brief Thread function callback storage.
   *
   * std::function allows storing any callable type set via setThreadFunc().
   *
   * @param h Handler that will manage allocated objects.
   * @param func Callback invoked for each object.
   */

  typedef std::function<void(CHandler &obj, CDescObject &desc)> allocobj_func_t;
  void alloc_objects(CHandler &h, const allocobj_func_t &func);

  /**
   * @brief Get object configuration at offset.
   *
   * @param offset Offset in 32-bit words.
   * @return Pointer to SObjectCfgData, nullptr if out of bounds.
   */

  SObjectCfg::SObjectCfgData *objectCfgAtOffset(size_t offset);

  /**
   * @brief Get configuration ID at offset.
   *
   * @param offset Offset in 32-bit words.
   * @return Pointer to UObjectCfgId, nullptr if out of bounds.
   */

  SObjectCfg::UObjectCfgId *objectCfgIdAtOffset(size_t offset);

  /**
   * @brief Print descriptor contents to console (debug).
   *
   * @param bin Pointer to binary descriptor data.
   * @param len Length in 32-bit words.
   */

  static void binDump(const uint32_t *bin, size_t len);

  /**
   * @brief Validate binary descriptor integrity.
   *
   * @param bin Pointer to binary descriptor data.
   * @param len Length in 32-bit words.
   * @return OK on success, negative error code on failure.
   */

  static int binValid(const uint32_t *bin, size_t len);

  /**
   * @brief Validate descriptor CRC32 checksum.
   *
   * @param bin Pointer to binary descriptor data.
   * @param len Length in 32-bit words.
   * @return True if CRC32 matches, false otherwise.
   */

  static bool binCheckValid(const uint32_t *bin, size_t len);

  /**
   * @brief Calculate and store descriptor CRC32 checksum.
   *
   * @param bin Pointer to binary descriptor data.
   * @param len Length in 32-bit words.
   * @return OK on success, negative error code on failure.
   */

  static int binCheckFill(uint32_t *bin, size_t len);

private:
  SDescriptorBin *bindesc;            // Pointer to loaded descriptor binary structure
  std::vector<CDescObject *> vobjcfg; // Vector of CDescObject wrappers
  size_t bindescLen;                  // Length of loaded descriptor in 32-bit words
  bool noIdleQuit;                    // no quit from idle flag

  /** @brief Parse metadata */

  void parseMeta();
  size_t objectWords() const;
  bool objectWordAt(size_t offset, uint32_t *value) const;
};
} // Namespace dawn
