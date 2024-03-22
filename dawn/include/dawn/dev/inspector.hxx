// dawn/include/dawn/dev/inspector.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>

#include "dawn/porting/config.hxx"

#ifdef CONFIG_DAWN_INSPECT

namespace dawn
{
// Forward declarations

class CObject;
class CIOCommon;
class CIOHandler;
class CProgHandler;
class CProtoHandler;

/**
 * @brief Global registry for object inspection.
 *
 * Provides read-only access to handlers for debugging and introspection.
 * Only compiled when CONFIG_DAWN_INSPECT is enabled. Uses singleton pattern
 * to avoid breaking handler independence while enabling cross-handler
 * inspection.
 *
 * This class follows the same architectural pattern as CDevDescriptor,
 * providing a graceful way to overcome architecture limits without
 * introducing direct coupling between handlers.
 */

class CDevInspector
{
public:
  /** @brief Handler types that can be registered. */

  enum EHandlerType
  {
    HANDLER_TYPE_IO = 0,    ///< I/O handler
    HANDLER_TYPE_PROG = 1,  ///< Program handler
    HANDLER_TYPE_PROTO = 2, ///< Protocol handler
    HANDLER_TYPE_MAX = 3    ///< Maximum handler count
  };

  /**
   * @brief Get singleton instance.
   *
   * Creates the singleton on first call, returns existing instance on
   * subsequent calls.
   *
   * @return Pointer to singleton instance.
   */

  static CDevInspector *getInst()
  {
    if (CDevInspector::singleton == nullptr)
      {
        CDevInspector::singleton = new CDevInspector();
      }

    return CDevInspector::singleton;
  }

  /**
   * @brief Destroy singleton.
   *
   * Deletes the singleton instance and sets pointer to nullptr.
   * Call this during framework shutdown.
   */

  static void destroy()
  {
    delete CDevInspector::singleton;
    CDevInspector::singleton = nullptr;
  }

  /**
   * @brief Register IO handler.
   *
   * Called by CIOHandler during initialization to register itself for
   * inspection. Ownership is NOT transferred - handler lifetime is managed
   * independently.
   *
   * @param handler Pointer to IO handler (must remain valid).
   */

  void registerIOHandler(const CIOHandler *handler);

  /**
   * @brief Register PROG handler.
   *
   * Called by CProgHandler during initialization to register itself for
   * inspection. Ownership is NOT transferred - handler lifetime is managed
   * independently.
   *
   * @param handler Pointer to PROG handler (must remain valid).
   */

  void registerProgHandler(const CProgHandler *handler);

  /**
   * @brief Register PROTO handler.
   *
   * Called by CProtoHandler during initialization to register itself for
   * inspection. Ownership is NOT transferred - handler lifetime is managed
   * independently.
   *
   * @param handler Pointer to PROTO handler (must remain valid).
   */

  void registerProtoHandler(const CProtoHandler *handler);

  /**
   * @brief Get number of IO objects.
   *
   * @return Count of IO objects, 0 if handler not registered.
   */

  size_t getIOCount() const;

  /**
   * @brief Get number of PROG objects.
   *
   * @return Count of PROG objects, 0 if handler not registered.
   */

  size_t getProgCount() const;

  /**
   * @brief Get number of PROTO objects.
   *
   * @return Count of PROTO objects, 0 if handler not registered.
   */

  size_t getProtoCount() const;

  /**
   * @brief Get IO object by index.
   *
   * @param index Object index (0 to getIOCount()-1).
   * @return Const pointer to IO object, nullptr if index out of range.
   */

  const CIOCommon *getIO(size_t index) const;

  /**
   * @brief Get PROG object by index.
   *
   * @param index Object index (0 to getProgCount()-1).
   * @return Const pointer to PROG object, nullptr if index out of range.
   */

  const CObject *getProg(size_t index) const;

  /**
   * @brief Get PROTO object by index.
   *
   * @param index Object index (0 to getProtoCount()-1).
   * @return Const pointer to PROTO object, nullptr if index out of range.
   */

  const CObject *getProto(size_t index) const;

  /**
   * @brief Find object by ID.
   *
   * Searches all handlers for object with given ID.
   *
   * @param objid Object ID to find.
   * @return Const pointer to object, nullptr if not found.
   */

  const CObject *findObject(uint32_t objid) const;

  /**
   * @brief Get IO bindings for PROG object.
   *
   * Returns list of IO objects bound to specified PROG.
   *
   * @param prog_index PROG object index.
   * @param[out] io_list Array to fill with IO pointers.
   * @param max_size Maximum entries in io_list.
   * @return Number of bindings returned, 0 if none or invalid index.
   */

  size_t getProgIOBindings(size_t prog_index, const CIOCommon **io_list, size_t max_size) const;

  /**
   * @brief Get object bindings for PROTO object.
   *
   * Returns list of objects (IO or PROG) bound to specified PROTO.
   *
   * @param proto_index PROTO object index.
   * @param[out] obj_list Array to fill with object pointers.
   * @param max_size Maximum entries in obj_list.
   * @return Number of bindings returned, 0 if none or invalid index.
   */

  size_t getProtoBindings(size_t proto_index, const CObject **obj_list, size_t max_size) const;

private:
  /** @brief Singleton instance. */

  static CDevInspector *singleton;

  /**
   * @brief Registered handlers.
   *
   * Const pointers provide read-only access to prevent modification of
   * handler state through inspector interface.
   */

  const CIOHandler *io_handler;
  const CProgHandler *prog_handler;
  const CProtoHandler *proto_handler;

  /**
   * @brief Constructor.
   *
   * Private to enforce singleton pattern. Initializes all handler pointers
   * to nullptr.
   */

  CDevInspector()
    : io_handler(nullptr)
    , prog_handler(nullptr)
    , proto_handler(nullptr)
  {
  }
};

} // Namespace dawn

#endif // CONFIG_DAWN_INSPECT
