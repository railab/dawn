// dawn/include/dawn/dev/descriptor.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "dawn/porting/config.hxx"

namespace dawn
{
/** @brief Descriptor interface available for IO. */

class CDevDescriptor
{
public:
  /** @brief Maximum number of descriptors that can be registered. */

  constexpr static int MAX_DESCRIPTORS = CONFIG_DAWN_DESC_SLOTS;
#if CONFIG_DAWN_DESC_SLOTS > 1
  constexpr static size_t SLOT_SIZE = CONFIG_DAWN_DESC_SLOT_SIZE;
#else
  constexpr static size_t SLOT_SIZE = 0;
#endif

  /** @brief Registered descriptor information. */

  struct descriptor_reg_s
  {
    void *ptr;  ///< Pointer to descriptor data
    size_t len; ///< Descriptor length in bytes
  } typedef SDescriptorReg;

  /**
   * @brief Get singleton instance.
   *
   * @return Pointer to CDevDescriptor instance.
   */

  static CDevDescriptor *getInst()
  {
    if (CDevDescriptor::singleton == nullptr)
      {
        CDevDescriptor::singleton = new CDevDescriptor();
      }

    return CDevDescriptor::singleton;
  }

  /** @brief Destroy singleton instance. */

  static void destroy()
  {
    delete CDevDescriptor::singleton;
    CDevDescriptor::singleton = nullptr;
  }

  /**
   * @brief Register descriptor data for an instance.
   *
   * @param inst Instance number.
   * @param reg Descriptor registration structure.
   * @return OK on success, negative error code on failure.
   */

  int regDescriptor(int inst, const SDescriptorReg &reg);

  /**
   * @brief Get registered descriptor data for an instance.
   *
   * @param inst Instance number.
   * @param reg Reference to structure where descriptor data will be stored.
   * @return OK on success, negative error code on failure.
   */

  int getDescriptor(int inst, SDescriptorReg &reg);

  /**
   * @brief Write descriptor bytes into a RAM slot.
   *
   * @param inst Slot index (must be >= 1).
   * @param data Source bytes.
   * @param offset Byte offset in slot.
   * @param len Number of bytes to write.
   * @return OK on success, negative error code on failure.
   */

  int writeSlotData(int inst, const void *data, size_t offset, size_t len);

  /**
   * @brief Get valid byte count currently stored in a RAM slot.
   *
   * @param inst Slot index.
   * @return Number of bytes written, or 0 for slot 0/empty/invalid slot.
   */

  size_t getSlotWritten(int inst) const;

  /**
   * @brief Reset a RAM slot.
   *
   * Clears data and marks slot length as zero.
   *
   * @param inst Slot index (must be >= 1).
   * @return OK on success, negative error code on failure.
   */

  int resetSlot(int inst);

private:
  /** @brief Singleton instance pointer. */

  static CDevDescriptor *singleton;

  /** @brief Registered descriptors storage. */

  SDescriptorReg regDesc[MAX_DESCRIPTORS] = {};

#if CONFIG_DAWN_DESC_SLOTS > 1
  /** @brief RAM descriptor slot data for slots 1..N-1. */

  std::array<std::array<uint8_t, SLOT_SIZE>, CONFIG_DAWN_DESC_SLOTS - 1> slotBuf = {};

  /** @brief Valid byte count for each RAM slot. */

  std::array<size_t, CONFIG_DAWN_DESC_SLOTS - 1> slotWritten = {};
#endif

  /** @brief Private constructor. */

  CDevDescriptor() = default;
};

} // Namespace dawn
