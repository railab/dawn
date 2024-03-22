// dawn/include/dawn/prog/factory.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/descobject.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
// Forward declaration

class CProgCommon;

/** @brief Abstract factory interface for PROG object creation. */

class IProgFactory
{
public:
  virtual ~IProgFactory() {};

  /**
   * @brief Create a PROG object from descriptor.
   *
   * @param[in] desc Descriptor containing PROG class and configuration.
   * @return CProgCommon* New protocol object, nullptr if class not supported.
   */

  virtual CProgCommon *create(CDescObject &desc) = 0;
};

/** @brief Built-in PROG factory implementation. */

class CProgFactory : public IProgFactory
{
public:
  /** @brief Constructor. */

  CProgFactory() = default;

  /** @brief Destructor. */

  ~CProgFactory() override = default;

  /**
   * @brief Create a PROG object from descriptor.
   *
   * @param desc Descriptor object defining Program to create.
   * @return Pointer to created CProgCommon subclass, nullptr if not supported.
   */

  CProgCommon *create(CDescObject &desc) override;
};
} // Namespace dawn
