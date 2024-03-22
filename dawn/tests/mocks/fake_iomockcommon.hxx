// dawn/tests/mocks/fake_iomockcommon.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/common.hxx"
#include "dawn/io/factory.hxx"
#include "dawn/porting/config.hxx"

using namespace dawn;

//***************************************************************************
// Description: Dummy IO with read and write support. Notifications not
// supported.
//***************************************************************************

class CIOMockCommon : public CIOCommon
{
public:
  static const uint32_t CIOMOCHFACTORY_ID =
    SObjectId::objectId(SObjectId::OBJTYPE_IO, 500, 0, 0, 0);

  explicit CIOMockCommon(CDescObject &desc)
    : CIOCommon(desc) {};

  ~CIOMockCommon() override {};

  static constexpr SObjectId::ObjectId objectId(uint16_t id)
  {
    return CIOMOCHFACTORY_ID + id;
  }

  int getDataImpl(IODataCmn &data, size_t len) override
  {
    return OK;
  };

  int setDataImpl(IODataCmn &data) override
  {
    return OK;
  };

  size_t getDataSize() const override
  {
    return 0;
  };

  size_t getDataDim() const override
  {
    return 0;
  };

  bool isRead() const override
  {
    return true;
  };

  bool isWrite() const override
  {
    return true;
  };

  bool isNotify() const override
  {
    return false;
  };

  bool isBatch() const override
  {
    return false;
  };

  int getDevno()
  {
    return this->getCmnDevno();
  };

  const uint32_t *getLimitMin()
  {
    return this->getCmnLimitMin();
  };

  const uint32_t *getLimitMax()
  {
    return this->getCmnLimitMax();
  };

  const uint32_t *getLimitStep()
  {
    return this->getCmnLimitStep();
  };

  size_t getLimitWords()
  {
    return this->getCmnLimitWords();
  };
};

//***************************************************************************
// Description: Dummy IO factory with deterministic behavior.
//***************************************************************************

class CIOMockFactoryFake : public IIOFactory
{
public:
  CIOMockFactoryFake() {};
  virtual ~CIOMockFactoryFake() override {};

  CIOCommon *create(CDescObject &desc) override
  {
    if (SObjectId::objectIdGetNoId(desc.getObjectIdV()) == CIOMockCommon::CIOMOCHFACTORY_ID)
      {
        return new CIOMockCommon(desc);
      }

    return nullptr;
  }
};
