// dawn/tests/mocks/spy_iomockcommon.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "mocks/fake_iomockcommon.hxx"
#include "test_mock_expect.hxx"

//***************************************************************************
// Description: Dummy IO factory that records calls for interaction checks.
//***************************************************************************

class CIOMockFactorySpy : public CIOMockFactoryFake
{
public:
  explicit CIOMockFactorySpy(MockTrace *trace)
    : trace_(trace) {};

  ~CIOMockFactorySpy() override {};

  CIOCommon *create(CDescObject &desc) override
  {
    MOCK_TRACE_CALL(trace_, MOCK_EVENT_CREATE, static_cast<int>(desc.getObjectIdV()), 0);
    return CIOMockFactoryFake::create(desc);
  }

private:
  MockTrace *trace_;
};
