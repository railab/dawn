// dawn/include/dawn/system/factory.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/descobject.hxx"
#include "dawn/system/common.hxx"

namespace dawn
{
/** @brief Interface for user-provided dev object factories. */

class ISystemFactory
{
public:
  virtual ~ISystemFactory() {};
  virtual CSystemCommon *create(CDescObject &desc) = 0;
};

/** @brief Built-in dev object factory. */

class CSystemFactory : public ISystemFactory
{
public:
  CSystemCommon *create(CDescObject &desc) override;
};
} // Namespace dawn
