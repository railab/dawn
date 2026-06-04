// dawn/include/dawn/io/handler.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/descriptor.hxx"
#include "dawn/common/generic_handler.hxx"
#include "dawn/common/handler.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/factory.hxx"
#include "dawn/porting/config.hxx"

#ifdef CONFIG_DAWN_IO_NOTIFY
#  include "dawn/io/notify_manager.hxx"
#endif

//***************************************************************************
// Public Classes
//***************************************************************************

namespace dawn
{
// Forward declaration

class CIOCommon;

/**
 * @brief Manages I/O object lifecycle and dispatch.
 *
 * Creates, initializes, starts, stops, and provides lookup for all I/O objects
 * in the system.
 */

class CIOHandler : public CGenericHandler<CIOCommon>
{
public:
  /**
   * @brief Construct I/O handler.
   *
   * Creates empty handler with no objects.
   */

  explicit CIOHandler()
    : userFactory(nullptr)
  {
  }

  /**
   * @brief Destructor.
   *
   * All I/O objects are destroyed here if deinitAll() was not called.
   */

  ~CIOHandler() = default;

  /**
   * @brief Initialize virtual I/O.
   *
   * Allocates internal data structures.
   *
   * @param desc Device descriptor containing I/O definitions.
   * @param f Optional user factory for custom I/O types (can be null).
   * @return OK on success, negative error code on failure.
   */

  int init(CDescriptor &desc, IIOFactory *f);

  /**
   * @brief Configure and initialize all I/O objects.
   *
   * Runs configure/init lifecycle passes for each I/O object in order.
   *
   * @return OK if all objects initialized, first error otherwise.
   */

  int initAll() override;

  /**
   * @brief De-initialize all I/O objects.
   *
   * Calls deinit() method on each I/O object in reverse order.
   *
   * @return OK if all objects deinitialized, first error otherwise.
   */

  int deinitAll() override
  {
    return CGenericHandler<CIOCommon>::deinitAll();
  }

  /**
   * @brief Start all I/O objects.
   *
   * Calls start() method on each I/O object and starts the notifier thread (if
   * CONFIG_DAWN_IO_NOTIFY enabled).
   *
   * @return OK if all started, first error otherwise.
   */

  int startAll() override;

  /**
   * @brief Stop all I/O objects.
   *
   * Stops the notifier thread (if CONFIG_DAWN_IO_NOTIFY enabled) and calls
   * stop() method on each I/O object in reverse order.
   *
   * @return OK if all stopped, first error otherwise.
   */

  int stopAll() override;

  /**
   * @brief Check if handler is currently running.
   *
   * @return True if startAll() has been called and stopAll() has not.
   */

  bool hasThread() const override
  {
    return CGenericHandler<CIOCommon>::hasThread();
  }

  /**
   * @brief Validate object ID.
   *
   * Checks if an ObjectID refers to a valid I/O object in this handler.
   *
   * @param obj Reference to ObjectID to validate.
   * @return True if object exists, false otherwise.
   */

  bool isObjectValid(SObjectId::UObjectId &obj) const override;

  /**
   * @brief Get object by ObjectID as CObject*.
   *
   * Returns object as CObject pointer for generic handler operations.
   *
   * @param id ObjectID to retrieve.
   * @return Pointer to CObject (CIOCommon*) or nullptr if not found.
   */

  CObject *getObject(const SObjectId::ObjectId id) override;

  /**
   * @brief Get I/O object by ObjectID as CIOCommon*.
   *
   * Implements IIOHandler interface.
   *
   * @param id Reference to ObjectID to retrieve.
   * @return Pointer to CIOCommon object or nullptr if not found.
   */

  CIOCommon *getIO(SObjectId::UObjectId &id) const;

  /**
   * @brief Bind special I/O objects (Config, Control, Trigger) to targets.
   *
   * Iterates all IOs and resolves bound target object IDs for special IO
   * types against all three handlers. Called after all initAll() phases.
   *
   * @param io I/O handler for resolving IO targets.
   * @param prog PROG handler for resolving program targets.
   * @param prot PROTO handler for resolving protocol targets.
   * @param dev DEV handler for resolving OBJTYPE_ANY dev-object targets.
   * @return OK on success, negative error code on failure.
   */

  int bindObjects(IHandler &io, IHandler &prog, IHandler &prot, IHandler &dev);

protected:
  /**
   * @brief Hook called for each I/O object during initAll().
   *
   * Called by CGenericHandler::initAll() after init() on each object.
   *
   * @param io Pointer to I/O object just initialized.
   */

  void onInitObject(CIOCommon *io) override;

  /**
   * @brief Get human-readable object type name for logging.
   *
   * @return String "IO" for I/O objects.
   */

  const char *getObjectTypeName() const override
  {
    return "IO";
  }

private:
  /**
   * @brief Built-in I/O object factory.
   *
   * Handles creation of standard I/O types (GPI, GPO, ADC, DAC, etc.).
   */

  CIOFactory factory;

  /**
   * @brief User-defined I/O factory (optional).
   *
   * Pointer to pluggable factory for custom I/O types.
   */

  IIOFactory *userFactory;

#ifdef CONFIG_DAWN_IO_NOTIFY
  /**
   * @brief Notifier manager for async I/O callbacks.
   *
   * Manages notifier instances, routing I/O objects to the appropriate
   * notifier based on configured type and priority.
   */

  CIONotifierManager notifyMgr;
#endif

  /**
   * @brief Allocate I/O objects from descriptor (static helper).
   *
   * Static method called to allocate objects as descriptor is parsed.
   *
   * @param obj CHandler (this handler) for allocation.
   * @param desc Descriptor object to allocate.
   */

  static void objallocPriv(CHandler &obj, CDescObject &desc);

  /**
   * @brief Allocate all I/O objects from descriptor.
   *
   * Parses descriptor and calls objallocPriv for each object entry, which then
   * calls create() to instantiate objects.
   *
   * @param desc Device descriptor containing I/O definitions.
   */

  int objalloc(CDescriptor &desc);

  /**
   * @brief Create I/O object from descriptor.
   *
   * Factory method to instantiate a IO object.
   *
   * @param desc Descriptor object defining I/O to create.
   * @return Pointer to created CIOCommon subclass, nullptr on failure.
   */

  CIOCommon *create(CDescObject &desc);
};
} // Namespace dawn
