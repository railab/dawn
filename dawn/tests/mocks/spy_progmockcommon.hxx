// dawn/tests/mocks/spy_progmockcommon.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "mocks/fake_progmockcommon.hxx"
#include "test_mock_expect.hxx"

//***************************************************************************
// Description: program factory spy records create calls before delegating.
//***************************************************************************

class CProgMockFactorySpy : public CProgMockFactoryFake
{
public:
  explicit CProgMockFactorySpy(MockTrace *trace)
    : trace_(trace) {};

  ~CProgMockFactorySpy() override {};

  CProgCommon *create(CDescObject &desc) override
  {
    MOCK_TRACE_CALL(trace_, MOCK_EVENT_CREATE, static_cast<int>(desc.getObjectIdV()), 0);
    return CProgMockFactoryFake::create(desc);
  }

private:
  MockTrace *trace_;
};
