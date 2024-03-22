// dawn/include/dawn/dawn.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/handler.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/handler.hxx"
#include "dawn/proto/handler.hxx"

namespace dawn
{
/**
 * @brief Main orchestrator class that integrates all Dawn components.
 *
 * CDawn is the primary entry point for using the Dawn framework.
 */

class CDawn
{
public:
  /**
   * @brief Construct CDawn framework.
   *
   * Creates a new Dawn framework instance with optional custom factories for
   * extensibility.
   *
   * @param iofactory Optional custom I/O object factory (nullptr uses the
   *   built-in factory).
   * @param progfactory Optional custom program object factory (nullptr uses
   *   the built-in factory).
   * @param protofactory Optional custom protocol object factory (nullptr uses
   *   the built-in factory).
   */

  explicit CDawn(IIOFactory *iofactory = nullptr,
                 IProgFactory *progfactory = nullptr,
                 IProtoFactory *protofactory = nullptr);

  /**
   * @brief Destructor.
   *
   * Cleans up all framework resources.
   */

  ~CDawn() = default;

  /**
   * @brief Load and initialize descriptor.
   *
   * Loads a binary device descriptor and automatically creates all configured
   * objects.
   *
   * @param bin Pointer to binary descriptor data (32-bit aligned).
   * @param len Length of descriptor in 32-bit words.
   * @return OK on success, negative error code on failure.
   */

  int load_descriptor(uint32_t *bin, size_t len);

  /**
   * @brief Start Dawn framework.
   *
   * Transitions all components from initialized to running state.
   *
   * @return OK on success, negative error code on failure.
   */

  int start(bool no_loop = false);

private:
  /** @brief Binary device descriptor parser and manager. */

  CDescriptor desc;

  /**
   * @brief I/O handler managing all I/O objects.
   *
   * Instantiates, initializes, and manages lifecycle of all I/O objects.
   */

  CIOHandler io;

  /**
   * @brief Program handler managing all program objects.
   *
   * Instantiates, initializes, and manages lifecycle of all program objects.
   */

  CProgHandler prog;

  /**
   * @brief Protocol handler managing all protocol objects.
   *
   * Instantiates, initializes, and manages lifecycle of all protocol handlers.
   */

  CProtoHandler proto;

  /**
   * @brief Custom I/O object factory (nullptr uses built-in).
   *
   * Allows application to provide custom I/O type implementations.
   */

  IIOFactory *userIOFactory;

  /**
   * @brief Custom program object factory (nullptr uses built-in).
   *
   * Allows application to provide custom program type implementations.
   */

  IProgFactory *userProgFactory;

  /**
   * @brief Custom protocol object factory (nullptr uses built-in).
   *
   * Allows application to provide custom protocol type implementations.
   */

  IProtoFactory *userProtoFactory;

#ifdef CONFIG_DAWN_DESC_SWITCH
  /**
   * @brief Load descriptor from registered descriptor slot.
   *
   * @param slot Descriptor slot index.
   * @return OK on success, negative error code on failure.
   */

  int load_descriptor_from_slot(uint8_t slot);
#endif
};
} // Namespace dawn
