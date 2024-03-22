// dawn/include/dawn/common/generic_handler.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <errno.h>

#include <algorithm>
#include <vector>

#include "dawn/common/descriptor.hxx"
#include "dawn/common/handler.hxx"
#include "dawn/common/objectid.hxx"
#include "dawn/debug.hxx"

namespace dawn
{
/** @brief Template handler for object lifecycle management. */

template<typename T>
class CGenericHandler : public CHandler
{
protected:
  /** @brief Vector of registered objects. */

  std::vector<T *> objects;
  int allocError = OK;

  /**
   * @brief Register object.
   *
   * @param obj Pointer to object to register.
   */

  int registerObject(T *obj)
  {
    DAWNASSERT(obj != nullptr, "invalid input");
    for (const T *tmp : objects)
      {
        if (tmp->getIdV() == obj->getIdV())
          {
            DAWNERR("Duplicate object ID 0x%" PRIx32 "\n", obj->getIdV());
            delete obj;
            allocError = -EEXIST;
            return allocError;
          }
      }
    objects.push_back(obj);
    return OK;
  }

  /**
   * @brief Hook for subclass-specific post-object-init handling.
   *
   * @param obj Pointer to object that was just initialized.
   */

  virtual void onInitObject(T *obj)
  {
  }

  /**
   * @brief Hook for subclass-specific pre-start operations.
   *
   * @param obj Pointer to object about to start.
   */

  virtual void onStartObject(T *obj)
  {
  }

  /** @brief Get object type name for logging. */

  virtual const char *getObjectTypeName() const
  {
    return "OBJECT";
  }

public:
  /**
   * @brief Destructor.
   *
   * Performs stop/delete cleanup only when lifecycle teardown is enabled.
   */

  ~CGenericHandler()
  {
#ifdef CONFIG_DAWN_LIFECYCLE_TEARDOWN
    for (auto it = objects.rbegin(); it != objects.rend(); ++it)
      {
        T *obj = *it;
        DAWNINFO("STOP %s 0x%" PRIx32 "\n", getObjectTypeName(), obj->getIdV());
        obj->stop();
      }

    for (T *obj : objects)
      {
        DAWNINFO("delete 0x%" PRIx32 "\n", obj->getIdV());
        delete obj;
      }

    objects.clear();
#endif
  }

  /**
   * @brief Configure all objects managed by this handler.
   *
   * @return OK on success, negative error code on failure.
   */

  int configureAll()
  {
    int ret;

    for (T *obj : objects)
      {
        DAWNINFO("CONFIG %s 0x%" PRIx32 "\n", getObjectTypeName(), obj->getIdV());
        ret = obj->configure();
        if (ret != OK)
          {
            DAWNERR("Failed to configure %s 0x%" PRIx32 " (error %d)\n",
                    getObjectTypeName(),
                    obj->getIdV(),
                    ret);
            return ret;
          }
      }

    return OK;
  }

  /**
   * @brief Run one-time init() for all configured objects.
   *
   * @return OK on success, negative error code on failure.
   */

  int initAll()
  {
    int ret;

    for (T *obj : objects)
      {
        DAWNINFO("INIT %s 0x%" PRIx32 "\n", getObjectTypeName(), obj->getIdV());
        ret = obj->init();
        if (ret != OK)
          {
            DAWNERR("Failed to init %s 0x%" PRIx32 " (error %d)\n",
                    getObjectTypeName(),
                    obj->getIdV(),
                    ret);
            return ret;
          }
        onInitObject(obj);
      }

    return OK;
  }

  /**
   * @brief De-initialize all objects managed by this handler.
   *
   * Returns OK without cleanup when lifecycle teardown is disabled.
   *
   * @return OK on success, negative error code on failure.
   */

  int deinitAll()
  {
#ifdef CONFIG_DAWN_LIFECYCLE_TEARDOWN
    int ret;
    int tmp;

    ret = OK;
    for (auto it = objects.rbegin(); it != objects.rend(); ++it)
      {
        T *obj = *it;
        DAWNINFO("DEINIT %s 0x%" PRIx32 "\n", getObjectTypeName(), obj->getIdV());
        tmp = obj->deinit();
        if (tmp != OK)
          {
            DAWNERR("Failed to deinit %s 0x%" PRIx32 " (error %d)\n",
                    getObjectTypeName(),
                    obj->getIdV(),
                    tmp);
            if (ret == OK)
              {
                ret = tmp;
              }
          }
      }

    for (T *obj : objects)
      {
        DAWNINFO("delete %s 0x%" PRIx32 "\n", getObjectTypeName(), obj->getIdV());
        delete obj;
      }

    objects.clear();

    return ret;
#else
    return OK;
#endif
  }

  /**
   * @brief Start all objects managed by this handler.
   *
   * @return OK on success, negative error code on failure.
   */

  int startAll()
  {
    int ret;

    for (T *obj : objects)
      {
        onStartObject(obj);
        DAWNINFO("START %s 0x%" PRIx32 "\n", getObjectTypeName(), obj->getIdV());
        ret = obj->start();
        if (ret != OK)
          {
            DAWNERR("Failed to start %s 0x%" PRIx32 " (error %d)\n",
                    getObjectTypeName(),
                    obj->getIdV(),
                    ret);
            return ret;
          }
      }

    return OK;
  }

  /**
   * @brief Stop all objects managed by this handler.
   *
   * Returns OK without stopping objects when lifecycle teardown is disabled.
   *
   * @return OK on success, negative error code on failure.
   */

  int stopAll()
  {
#ifdef CONFIG_DAWN_LIFECYCLE_TEARDOWN
    int ret;

    for (auto it = objects.rbegin(); it != objects.rend(); ++it)
      {
        T *obj = *it;
        DAWNINFO("STOP %s 0x%" PRIx32 "\n", getObjectTypeName(), obj->getIdV());
        ret = obj->stop();
        if (ret != OK)
          {
            DAWNERR("Failed to stop %s 0x%" PRIx32 " (error %d)\n",
                    getObjectTypeName(),
                    obj->getIdV(),
                    ret);
            return ret;
          }
      }

    return OK;
#else
    return OK;
#endif
  }

  /**
   * @brief Check if any thread is currently running.
   *
   * @return True if at least one object is running.
   */

  bool hasThread() const
  {
    return std::any_of(
      objects.begin(), objects.end(), [](const T *obj) { return obj->hasThread(); });
  }

  /** @brief Get total number of registered objects. */

  size_t getObjectCount() const
  {
    return objects.size();
  }

#ifdef CONFIG_DAWN_INSPECT
  /**
   * @brief Get read-only access to all objects.
   *
   * @return Const reference to object vector.
   */

  const std::vector<T *> &getObjects() const
  {
    return objects;
  }
#endif // CONFIG_DAWN_INSPECT

  /**
   * @brief Get object at index.
   *
   * @param index Zero-based index.
   * @return Pointer to object, nullptr if out of bounds.
   */

  T *getObjectAt(size_t index) const
  {
    return (index < objects.size()) ? objects[index] : nullptr;
  }

  /**
   * @brief Get object by ObjectID.
   *
   * @param id ObjectID to retrieve.
   * @return Pointer to object, nullptr if not found.
   */

  T *getObjectById(const SObjectId::UObjectId &id) const
  {
    auto it = std::find_if(
      objects.begin(), objects.end(), [&id](const T *obj) { return obj->getIdV() == id.v; });
    return (it != objects.end()) ? *it : nullptr;
  }

  /**
   * @brief Allocate objects from descriptor.
   *
   * Generic descriptor allocation using callback pattern.
   *
   * @param desc Device descriptor containing object definitions.
   * @param func Static callback: void func(CHandler& handler, CDescObject&).
   */

  int objalloc(CDescriptor &desc, CDescriptor::allocobj_func_t func)
  {
    allocError = OK;
    desc.alloc_objects(*this, func);
    return allocError;
  }
};

} // Namespace dawn
