// dawn/include/dawn/common/object.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>

#include "dawn/common/descobject.hxx"
#include "dawn/common/objectcfg.hxx"
#include "dawn/common/objectid.hxx"
#include "dawn/porting/config.hxx"

#ifndef CONFIG_DAWN_OBJECT_NAME_SIZE
#  define CONFIG_DAWN_OBJECT_NAME_SIZE 16
#endif

namespace dawn
{
/**
 * @brief Base class for all Dawn objects (IOs, Programs, Protocols).
 *
 * This class must be inherited by all other Dawn objects.
 */

class CObject
{
public:
  /** @brief Object operational state returned by getState() */

  enum
  {
    STATE_STOPPED = 0, ///< Object is stopped
    STATE_RUNNING = 1  ///< Object is running
  } typedef EObjectState;

  /** @brief Control command identifiers for control() */

  enum
  {
    CMD_RESET = 0,    ///< Reset object internal state
    CMD_TRIGGER1 = 1, ///< Object-specific trigger slot 1
    CMD_TRIGGER2 = 2, ///< Object-specific trigger slot 2
    CMD_TRIGGER3 = 3  ///< Object-specific trigger slot 3
  };

  /**
   * @brief Construct a CObject from a descriptor.
   *
   * @param desc Descriptor object defining this object's configuration.
   */

  explicit CObject(CDescObject &desc)
    : started(false)
    , objdesc(desc)
    , uobjid(desc.getObjectId())
  {
  }

  virtual ~CObject() = default;

  /**
   * @brief Configure object from descriptor data.
   *
   * Called during framework setup before dependency binding. Implementations
   * should parse/validate descriptor configuration and store object settings,
   * but avoid one-time allocations that depend on bound objects.
   *
   * @return OK on success, negative error code on failure.
   */

  virtual int configure()
  {
    return OK;
  };

  /**
   * @brief One-time initialize object after bindings are resolved.
   *
   * Called once during framework setup after dependency binding. Allocate
   * lifetime resources here (buffers, register maps, etc.).
   *
   * @return OK on success, negative error code on failure.
   */

  virtual int init()
  {
    return OK;
  };

  /**
   * @brief De-initialize object.
   *
   * Clean up resources allocated during init().
   *
   * @return OK on success, negative error code on failure.
   */

  virtual int deinit()
  {
    return OK;
  };

  /**
   * @brief Start object.
   *
   * Tracks operational state and calls doStart(). This phase may run many
   * times during an object's lifetime and should not perform one-time
   * allocation.
   * Sets started on successful doStart() return.
   *
   * @return OK on success, negative error code on failure.
   */

  int start()
  {
    int ret;

    if (started)
      {
        return OK;
      }

    ret = doStart();
    if (ret == OK)
      {
        started = true;
      }

    return ret;
  };

  /**
   * @brief Stop object.
   *
   * Tracks operational state and calls doStop(). This phase may run many
   * times during an object's lifetime and should not perform one-time
   * deallocation.
   * Always clears started regardless of doStop() return value.
   *
   * @return OK on success, negative error code on failure.
   */

  int stop()
  {
    int ret;

    if (!started)
      {
        return OK;
      }

    ret = doStop();
    started = false;
    return ret;
  };

  /**
   * @brief Check if a background thread is active.
   *
   * Used by Dawn's main loop to detect when clean exit is possible.
   * Objects without a background thread must return false.
   * Independent of operational state tracked via getState().
   *
   * @return True if a background thread is active, false otherwise.
   */

  virtual bool hasThread() const
  {
    return false;
  }

  /**
   * @brief Get current operational state.
   *
   * Default returns STATE_RUNNING after successful start(), STATE_STOPPED
   * otherwise. Objects with richer states (e.g. paused) should override.
   *
   * @return Current EObjectState value.
   */

  virtual EObjectState getState() const
  {
    return started ? STATE_RUNNING : STATE_STOPPED;
  }

  /**
   * @brief Execute a trigger command.
   *
   * @param cmd Command identifier (EObjectCmd value).
   * @return OK on success, -ENOTSUP if not supported.
   */

  virtual int trigger(uint8_t cmd)
  {
    UNUSED(cmd);
    return -ENOTSUP;
  }

  /**
   * @brief Get object identifier as union structure.
   *
   * @return Reference to UObjectId structure.
   */

  SObjectId::UObjectId getId() const;

  /**
   * @brief Get object identifier as raw 32-bit value.
   *
   * @return ObjectID as uint32_t.
   */

  SObjectId::ObjectId getIdV() const;

  /**
   * @brief Check if configuration flag is set.
   *
   * @return True if this object has configuration data.
   */

  bool getCfgFlag() const;

  /**
   * @brief Get object type field.
   *
   * @return Object type (OBJTYPE_IO, OBJTYPE_PROTO, OBJTYPE_PROG).
   */

  uint8_t getType() const;

  /**
   * @brief Get object class field.
   *
   * @return Object class identifier (0-511).
   */

  uint16_t getCls() const;

  /**
   * @brief Get data type field.
   *
   * @return Data type (DTYPE_UINT32, DTYPE_FLOAT, etc.).
   */

  uint8_t getDtype() const;

  /**
   * @brief Get type-specific flags field.
   *
   * @return Flags value (0-3).
   */

  uint8_t getFlags() const;

  /**
   * @brief Get instance/private data field.
   *
   * @return Instance ID (0-16383).
   */

  uint16_t getPriv() const;

  /**
   * @brief Get descriptor object for this object.
   *
   * @return Reference to associated CDescObject.
   */

  CDescObject &getDesc();

  /**
   * @brief Get size of this object's data type.
   *
   * @return Size in bytes for the configured data type.
   */

  size_t getDtypeSize() const;

  /**
   * @brief Set object configuration item.
   *
   * Updates a configuration item for this object.
   *
   * @param objcfg Configuration ID specifying which config item to update.
   * @param data Pointer to configuration data.
   * @param len Length of configuration data in 32-bit words.
   * @return OK on success, negative error code on failure.
   */

  int setObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len);

  /**
   * @brief Get object configuration item.
   *
   * Retrieves configuration data for this object.
   *
   * @param objcfg Configuration ID specifying which config item to retrieve.
   * @param data Pointer to buffer for configuration data.
   * @param len Length of buffer in 32-bit words.
   * @return OK on success, negative error code on failure.
   */

  int getObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len);

  /**
   * @brief Default descriptor validation failure codes.
   *
   * Negative values identify which length check failed in
   * descValidDefault().
   */

  enum
  {
    DESCVALID_ERR_LEN_ALIGN = -1,
    DESCVALID_ERR_NO_OBJSIZE = -2,
    DESCVALID_ERR_NO_CFG_HEADER = -3,
    DESCVALID_ERR_CFG_TRUNCATED = -4,
    DESCVALID_ERR_CFG_DATA = -6
  };

  /**
   * @brief Default descriptor validation method.
   *
   * Provides basic validation that only checks if descriptor lengths are
   * correct.
   *
   * @param data Descriptor data pointer.
   * @param len Length of descriptor data in 32-bit words.
   * @return OK on success, negative error code indicating validation stage.
   */

  static int descValidDefault(const uint32_t *data, size_t len);

  /**
   * @brief Validate entire descriptor.
   *
   * Validates a descriptor binary object-by-object using default checks.
   *
   * @param desc Descriptor binary pointer.
   * @param len Descriptor length in 32-bit words.
   * @return OK on success, negative error code indicating validation stage.
   */

  static int validateDesc(const uint32_t *desc, size_t len);

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  /**
   * @brief Get class name string (without instance number).
   *
   * Each derived class must implement this to return its class name.
   * For example: "adc", "pwm", "min", "shell", etc.
   *
   * @return Const pointer to class name string.
   */

  virtual const char *getClassNameStr() const = 0;

  /**
   * @brief Get human-readable object name.
   *
   * Returns name in format "classname_instance" (e.g., "adc_0", "pwm_0").
   * Only available when CONFIG_DAWN_OBJECT_HAS_NAME is enabled.
   *
   * @return Const pointer to null-terminated name string.
   */

  const char *getName() const;
#endif // CONFIG_DAWN_OBJECT_HAS_NAME

protected:
  /**
   * @brief Pre-update hook for runtime configuration writes.
   *
   * Called by setObjConfig() before descriptor-backed storage is updated.
   * Derived classes may validate incoming data and prepare/apply runtime
   * side effects. Returning a negative error rejects the update.
   *
   * @param objcfg Configuration ID that will be updated.
   * @param data Pointer to new config words.
   * @param len Number of config words.
   * @return OK on success, or negative error code to reject update.
   */

  virtual int onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
  {
    UNUSED(objcfg);
    UNUSED(data);
    UNUSED(len);
    return OK;
  }

  /**
   * @brief Start implementation hook.
   *
   * Override in derived classes instead of start().
   *
   * @return OK on success, negative error code on failure.
   */

  virtual int doStart()
  {
    return OK;
  }

  /**
   * @brief Stop implementation hook.
   *
   * Override in derived classes instead of stop().
   *
   * @return OK on success, negative error code on failure.
   */

  virtual int doStop()
  {
    return OK;
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  /**
   * @brief Human-readable object name.
   *
   * Stores name in format "classname_instance" for debugging and display.
   * Size configured by CONFIG_DAWN_OBJECT_NAME_SIZE (default 16).
   * Mutable to allow lazy initialization in getName().
   */

  mutable char name[CONFIG_DAWN_OBJECT_NAME_SIZE] = {};
#endif

private:
  bool started; // Operational started state, set by start()/stop() wrappers.
  CDescObject &objdesc;
  SObjectId::UObjectId uobjid;
};

} // Namespace dawn
