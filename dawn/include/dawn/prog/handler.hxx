// dawn/include/dawn/prog/handler.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vector>

#include "dawn/common/descriptor.hxx"
#include "dawn/common/generic_handler.hxx"
#include "dawn/common/handler.hxx"
#include "dawn/io/handler.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"
#include "dawn/prog/factory.hxx"

namespace dawn
{
/**
 * @brief Manages Programs object lifecycle and dispatch.
 *
 * Creates, initializes, starts, stops, and provides lookup for all Programs
 * objects in the system.
 */

class CProgHandler : public CGenericHandler<CProgCommon>
{
public:
  /**
   * @brief Construct the Programs handler.
   *
   * Creates an empty handler with no Programs objects.
   */

  CProgHandler()
    : userFactory(nullptr)
    , ioHandler(nullptr)
  {
  }

  /**
   * @brief Destruct the Programs handler.
   *
   * Destroys all managed Programs objects.
   */

  ~CProgHandler() = default;

  /**
   * @brief Initialize the Programs handler.
   *
   * Performs full Programs handler initialization.
   *
   * @param[in] desc Device descriptor.
   * @param[in] io I/O handler for I/O object lookup.
   * @param[in] f Optional user factory, or nullptr for built-in only.
   * @return OK on success, or negative error code on failure.
   */

  int init(CDescriptor &desc, CIOHandler *io, IProgFactory *f);

  /**
   * @brief Configure, bind, and initialize all Program objects.
   *
   * Performs the Program lifecycle setup sequence:
   * configure() for each object, bind IO dependencies, then object init().
   *
   * @return OK if all initialized, or first error code encountered.
   */

  int initAll() override;

  /**
   * @brief Deinitialize all Programs objects.
   *
   * Called during shutdown to clean up all Programs objects.
   *
   * @return OK if all deinitialized successfully.
   */

  int deinitAll() override
  {
    return CGenericHandler<CProgCommon>::deinitAll();
  }

  /**
   * @brief Start all Programs objects.
   *
   * Activates all Programs for operation (e.g., start sampling thread, enable
   * event processing).
   *
   * @return OK if all started, or first error code encountered.
   */

  int startAll() override
  {
    return CGenericHandler<CProgCommon>::startAll();
  }

  /**
   * @brief Stop all Programs objects.
   *
   * Deactivates all Programs (e.g., stop sampling thread, disable event
   * processing).
   *
   * @return OK if all stopped successfully.
   */

  int stopAll() override
  {
    return CGenericHandler<CProgCommon>::stopAll();
  }

  /**
   * @brief Check if any Programs are running.
   *
   * Returns current operational state.
   *
   * @return true if any Programs is currently running.
   */

  bool hasThread() const override
  {
    return CGenericHandler<CProgCommon>::hasThread();
  }

  /**
   * @brief Validate object ID in handler collection.
   *
   * Checks if the given object ID corresponds to a valid Program in the
   * handler's collection.
   *
   * @param[in] obj Object ID to validate.
   * @return true if object exists in handler, false otherwise.
   */

  bool isObjectValid(SObjectId::UObjectId &obj) const override;

  /**
   * @brief Get a Programs object by object ID.
   *
   * Generic lookup method returning any CObject pointer.
   *
   * @param[in] id Object ID to look up.
   * @return CObject pointer (Programs object), or nullptr if not found.
   */

  CObject *getObject(const SObjectId::ObjectId id) override;

  /**
   * @brief Get a Programs object by object ID (type-safe).
   *
   * Retrieves a Programs object from the handler's collection.
   *
   * @param[in] id Object ID to look up.
   * @return CProgCommon pointer, or nullptr if not found.
   */

  CProgCommon *getProg(SObjectId::UObjectId &id) const;

protected:
  /**
   * @brief Get object type name for logging.
   *
   * Used in debug output and error messages.
   *
   * @return String "Programs" identifying this handler type.
   */

  const char *getObjectTypeName() const override
  {
    return "PROG";
  }

private:
  /**
   * @brief Built-in Programs factory.
   *
   * Creates standard Programs types.
   */

  CProgFactory factory;

  /**
   * @brief User-provided Programs factory (optional).
   *
   * If provided by application, used for custom Programs types.
   */

  IProgFactory *userFactory;

  /**
   * @brief Reference to I/O handler.
   *
   * Used to look up source I/O objects during Programs binding.
   */

  CIOHandler *ioHandler;

  /**
   * @brief Create a Programs object from descriptor.
   *
   * Factory method to instantiate a Programs object.
   *
   * @param[in] desc Programs descriptor.
   * @return CProgCommon pointer, or nullptr on failure.
   */

  CProgCommon *create(CDescObject &desc);

  /**
   * @brief Look up I/O object by ID.
   *
   * Helper to retrieve I/O objects from the I/O handler.
   *
   * @param[in] id I/O object ID.
   * @return CIOCommon pointer, or nullptr if not found.
   */

  CIOCommon *getIO(SObjectId::ObjectId id);

  /**
   * @brief Private callback for object allocation.
   *
   * Static method used during descriptor parsing to allocate individual
   * Programs objects.
   *
   * @param[in] obj Handler (cast to CHandler for context).
   * @param[in] desc Programs descriptor.
   */

  static void objallocPriv(CHandler &obj, CDescObject &desc);

  /**
   * @brief Allocate all Programs objects from descriptor.
   *
   * Iterates descriptor for Programs objects and creates each one.
   *
   * @param[in] desc Device descriptor containing Programs objects.
   */

  int objalloc(CDescriptor &desc);
};
} // Namespace dawn
