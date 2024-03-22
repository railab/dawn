// dawn/include/dawn/common/handler.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/object.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Common interface for all handler implementations.
 *
 * Handlers manage collections of objects of a specific type (IOs, Programs,
 * Protocols).
 */

class IHandler
{
public:
  virtual ~IHandler() {};

  /**
   * @brief Configure and initialize all objects managed by this handler.
   *
   * Called during framework setup. Handlers may internally perform multiple
   * phases (for example configure -> bind -> init) before objects are ready
   * for runtime start().
   *
   * @return OK on success, negative error code if any object fails to init.
   */

  virtual int initAll() = 0;

  /**
   * @brief De-initialize all objects managed by this handler.
   *
   * Called during framework shutdown to de-initialize all registered objects
   * in reverse order.
   *
   * @return OK on success, negative error code if any object fails to deinit.
   */

  virtual int deinitAll() = 0;

  /**
   * @brief Start all objects managed by this handler.
   *
   * Called after all objects in the system have been initialized.
   *
   * @return OK on success, negative error code if any object fails to start.
   */

  virtual int startAll() = 0;

  /**
   * @brief Stop all objects managed by this handler.
   *
   * Stops all registered objects.
   *
   * @return OK on success, negative error code if any object fails to stop.
   */

  virtual int stopAll() = 0;

  /**
   * @brief Check if thread is currently running.
   *
   * Checks if a thread has been started and has not yet completed.
   *
   * @return True if at least one registered object is running, false if all.
   */

  virtual bool hasThread() const = 0;

  /**
   * @brief Validate if object ID is valid for this handler.
   *
   * Checks whether a given object ID represents a valid object managed by this
   * handler.
   *
   * @param obj Object ID to validate.
   * @return True if object ID is valid for this handler, false otherwise.
   */

  virtual bool isObjectValid(SObjectId::UObjectId &obj) const = 0;

  /**
   * @brief Get object from this handler by ID.
   *
   * Retrieves a specific object by its ID.
   *
   * @param id Object ID.
   * @return Pointer to CObject if found, nullptr if not found.
   */

  virtual CObject *getObject(const SObjectId::ObjectId id) = 0;
};

/**
 * @brief Base implementation of IHandler interface.
 *
 * Provides a basic handler implementation that can be specialized by
 * subclasses for specific object types (I/O, Program, Protocol handlers).
 */

class CHandler : public IHandler
{
};
} // Namespace dawn
