// dawn/include/dawn/proto/factory.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/descobject.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/proto/common.hxx"

namespace dawn
{
/** @brief Abstract factory interface for protocol creation. */

class IProtoFactory
{
public:
  virtual ~IProtoFactory() {};

  /**
   * @brief Create a protocol object.
   *
   * Factory method that creates an appropriate protocol implementation based
   * on the descriptor's protocol class.
   *
   * @param[in] desc Descriptor containing PROTO class and configuration.
   * @return CProtoCommon* New protocol object, nullptr if class not supported.
   */

  virtual CProtoCommon *create(CDescObject &desc) = 0;
};

/** @brief Built-in protocol factory. */

class CProtoFactory : public IProtoFactory
{
public:
  /** @brief Constructor. */

  CProtoFactory() = default;

  /** @brief Destructor. */

  ~CProtoFactory() override = default;

  /**
   * @brief Create a protocol object from descriptor.
   *
   * Factory method that creates an appropriate protocol implementation based
   * on the descriptor's protocol class.
   *
   * @param desc Descriptor object defining Protocol to create.
   * @return Pointer to created CProtoCommon subclass, nullptr if not supported.
   */

  CProtoCommon *create(CDescObject &desc) override;
};
} // Namespace dawn
