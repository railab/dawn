// dawn/include/dawn/proto/handler.hxx
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
#include "dawn/proto/common.hxx"
#include "dawn/proto/factory.hxx"

namespace dawn
{
/**
 * @brief Manages PROTO object lifecycle and dispatch.
 *
 * Creates, initializes, starts, stops, and provides lookup for all PROTO
 * objects in the system.
 */

class CProtoHandler : public CGenericHandler<CProtoCommon>
{
public:
  /**
   * @brief Constructor.
   *
   * Initializes the protocol handler with default state.
   */

  CProtoHandler()
    : userFactory(nullptr)
    , ioHandler(nullptr)
  {
  }

  /**
   * @brief Destructor.
   *
   * Cleans up all protocol objects and handler state.
   */

  ~CProtoHandler() = default;

  /**
   * @brief Initialize protocol handler and instances.
   *
   * Creates protocol objects from the descriptor and binds them to I/O.
   *
   * @param[in] desc Device descriptor with protocol configurations.
   * @param[in] io I/O handler reference for protocol I/O access.
   * @param[in] f User-provided factory (nullptr = use built-in factory).
   * @return int 0 on success, negative error code on failure.
   */

  int init(CDescriptor &desc, CIOHandler *io, IProtoFactory *f);

  /**
   * @brief Configure, bind, and initialize all protocol objects.
   *
   * Performs the Protocol lifecycle setup sequence:
   * configure() for each object, bind IO dependencies, then object init().
   *
   * @return int 0 on success, negative error code on failure.
   */

  int initAll() override;

  /**
   * @brief Deinitialize all protocol objects.
   *
   * Calls deinit() on each protocol object.
   *
   * @return int 0 on success, negative error code on failure.
   */

  int deinitAll() override
  {
    return CGenericHandler<CProtoCommon>::deinitAll();
  }

  /**
   * @brief Start all protocol objects.
   *
   * Calls start() on each protocol object.
   *
   * @return int 0 on success, negative error code on failure.
   */

  int startAll() override
  {
    return CGenericHandler<CProtoCommon>::startAll();
  }

  /**
   * @brief Stop all protocol objects.
   *
   * Calls stop() on each protocol object.
   *
   * @return int 0 on success, negative error code on failure.
   */

  int stopAll() override
  {
    return CGenericHandler<CProtoCommon>::stopAll();
  }

  /**
   * @brief Check if any protocol is running.
   *
   * @return bool true if at least one protocol is active, false otherwise.
   */

  bool hasThread() const override
  {
    return CGenericHandler<CProtoCommon>::hasThread();
  }

  /**
   * @brief Validate protocol object ID.
   *
   * Checks if a protocol object ID is valid and registered.
   *
   * @param[in] obj Protocol object ID.
   * @return bool true if object exists and is valid.
   */

  bool isObjectValid(SObjectId::UObjectId &obj) const override;

  /**
   * @brief Get protocol object by ID.
   *
   * Retrieves a protocol instance by its object ID.
   *
   * @param[in] id Protocol object ID.
   * @return CObject* Pointer to protocol object, nullptr if not found.
   */

  CObject *getObject(const SObjectId::ObjectId id) override;

  /**
   * @brief Get protocol object interface.
   *
   * Retrieves a protocol instance for external use.
   *
   * @param[in] id Protocol object ID.
   * @return CProtoCommon* Protocol object pointer, nullptr if not found.
   */

  CProtoCommon *getProto(SObjectId::UObjectId &id) const;

protected:
  /**
   * @brief Get object type name for logging.
   *
   * @return const char* "PROTO" for log messages.
   */

  const char *getObjectTypeName() const override
  {
    return "PROTO";
  }

private:
  /** @brief Built-in protocol factory. */

  CProtoFactory factory;

  /** @brief User-provided factory (nullptr = use built-in). */

  IProtoFactory *userFactory;

  /** @brief Reference to I/O handler for protocol I/O access. */

  CIOHandler *ioHandler;

  /**
   * @brief Static callback for object allocation.
   *
   * Called by descriptor parser for each protocol item found.
   *
   * @param[in] obj Handler reference (actually CProtoHandler*).
   * @param[in] desc Descriptor object to allocate.
   */

  static void objallocPriv(CHandler &obj, CDescObject &desc);

  /**
   * @brief Allocate protocol object from descriptor.
   *
   * Creates a protocol object based on descriptor and adds it to the object
   * list.
   *
   * @param[in] desc Descriptor object with protocol configuration.
   */

  int objalloc(CDescriptor &desc);

  /**
   * @brief Create a protocol object.
   *
   * Factory method that creates an appropriate protocol implementation based
   * on the descriptor's protocol class.
   *
   * @param[in] desc Descriptor object with protocol class and config.
   * @return CProtoCommon* New protocol object, nullptr on failure.
   */

  CProtoCommon *create(CDescObject &desc);

  /**
   * @brief Get I/O object from I/O handler.
   *
   * Helper method to retrieve I/O objects for protocol I/O binding.
   *
   * @param[in] id I/O object ID.
   * @return CIOCommon* I/O object pointer, nullptr if not found.
   */

  CIOCommon *getIO(SObjectId::ObjectId id);
};
} // Namespace dawn
