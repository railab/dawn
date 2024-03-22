// dawn/include/dawn/io/factory.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/descobject.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;

/** @brief Abstract factory interface for extensible I/O object creation. */

class IIOFactory
{
public:
  virtual ~IIOFactory() {};

  /**
   * @brief Create I/O object from descriptor.
   *
   * Factory method to instantiate a IO object.
   *
   * @param[in] desc Descriptor containing IO class and configuration.
   * @return CIoCommon* New protocol object, nullptr if class not supported.
   */

  virtual CIOCommon *create(CDescObject &desc) = 0;
};

/** @brief Built-in I/O object factory for standard I/O types. */

class CIOFactory : public IIOFactory
{
public:
  CIOFactory() = default;
  ~CIOFactory() override = default;

  /**
   * @brief Create I/O object from descriptor.
   *
   * Factory method to instantiate a IO object.
   *
   * @param desc Descriptor object defining I/O to create.
   * @return Pointer to created CIOCommon subclass, nullptr if not supported.
   */

  CIOCommon *create(CDescObject &desc) override;
};
} // Namespace dawn
