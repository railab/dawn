// dawn/include/dawn/common/bindable.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <map>

#include "dawn/common/object.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;

/**
 * @brief Base object with bindable object map support.
 *
 * Provides shared object binding helpers used by protocol/program objects.
 */

class CBindableObject : public CObject
{
public:
  /**
   * @brief Construct a CBindableObject from a descriptor.
   *
   * @param desc Descriptor object defining this object's configuration.
   */

  explicit CBindableObject(CDescObject &desc)
    : CObject(desc)
  {
  }

  /**
   * @brief Get the I/O map for this object.
   *
   * @return Pointer to a map of object IDs to I/O objects.
   */

  const std::map<SObjectId::ObjectId, CIOCommon *> &getIOMap() const;

  /**
   * @brief Set an item in the object map.
   *
   * @param id Object ID to set.
   * @param obj Object pointer to associate with the ID.
   */

  void setObjectMapItem(SObjectId::ObjectId id, CObject *obj);

  /**
   * @brief Get an I/O object by ID.
   *
   * @param id Object ID to retrieve.
   * @return Pointer to the I/O object, or nullptr if not found.
   */

  CIOCommon *getIO(SObjectId::ObjectId id);

  /**
   * @brief Get an object by ID.
   *
   * @param id Object ID to retrieve.
   * @return Pointer to the object, or nullptr if not found.
   */

  CObject *getObject(SObjectId::ObjectId id);

private:
  /**
   * @brief Set an item in the I/O map.
   *
   * @param id Object ID to set.
   * @param io I/O object pointer to associate with the ID.
   */

  void setIOMapItem(SObjectId::ObjectId id, CIOCommon *io);

  /** @brief object IO map */

  std::map<SObjectId::ObjectId, CIOCommon *> ioMap;
};
} // Namespace dawn
