// dawn/tests/include/test_mockhandler.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/handler.hxx"
#include "dawn/porting/config.hxx"

using namespace dawn;

class CMockHandler : public CHandler
{
public:
  int initAll() override
  {
    return OK;
  };

  int deinitAll() override
  {
    return OK;
  };

  int startAll() override
  {
    return OK;
  };

  int stopAll() override
  {
    return OK;
  };

  bool hasThread() const override
  {
    return false;
  };

  bool isObjectValid(SObjectId::UObjectId &obj) const override
  {
    return true;
  };

  CObject *getObject(const SObjectId::ObjectId id)
  {
    return NULL;
  }
};
