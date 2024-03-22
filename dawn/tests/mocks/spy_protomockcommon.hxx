// dawn/tests/mocks/spy_protomockcommon.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "mocks/fake_protomockcommon.hxx"
#include "test_mock_expect.hxx"

//***************************************************************************
// Description: protocol factory spy records create calls before delegating.
//***************************************************************************

class CProtoMockFactorySpy : public CProtoMockFactoryFake
{
public:
  explicit CProtoMockFactorySpy(MockTrace *trace)
    : trace_(trace) {};

  ~CProtoMockFactorySpy() override {};

  CProtoCommon *create(CDescObject &cfg) override
  {
    MOCK_TRACE_CALL(trace_, MOCK_EVENT_CREATE, static_cast<int>(cfg.getObjectIdV()), 0);
    return CProtoMockFactoryFake::create(cfg);
  }

private:
  MockTrace *trace_;
};
