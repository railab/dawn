// dawn/tests/mocks/fake_progmockcommon.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"
#include "dawn/prog/factory.hxx"

using namespace dawn;

//***************************************************************************
// Description: program mock tracks start/stop and configured IO bindings.
//***************************************************************************

class CProgMockCommon : public CProgCommon
{
public:
  static const uint32_t CPROGMOCHFACTORY_ID =
    SObjectId::objectId(SObjectId::OBJTYPE_PROG, 500, 0, 0, 0);

  bool running;

  enum prog_mock_cfg_e
  {
    PROG_MOCK_CFG_FIRST = 0,
    PROG_MOCK_CFG_IOBIND = 1,
    PROG_MOCK_CFG_LAST = 63
  } typedef EProgMockCfg;

  explicit CProgMockCommon(CDescObject &desc)
    : CProgCommon(desc)
  {
    this->running = false;
    this->configure(this->getDesc());
  };

  ~CProgMockCommon() override {};

  static constexpr SObjectId::ObjectId objectId(uint16_t id)
  {
    return CPROGMOCHFACTORY_ID + id;
  }

  int doStart() override
  {
    DAWNINFO("START 0x%" PRIx32 "\n", this->getIdV());
    this->running = true;
    return OK;
  }

  int doStop() override
  {
    DAWNINFO("STOP 0x%" PRIx32 "\n", this->getIdV());
    this->running = false;
    return OK;
  }

  bool hasThread() const override
  {
    return this->running;
  }

  void configure(const CDescObject &desc)
  {
    SObjectCfg::SObjectCfgItem *item = nullptr;
    size_t offset = 0;

    for (size_t i = 0; i < desc.getSize(); i++)
      {
        item = desc.objectCfgItemAtOffset(offset);

        switch (item->cfgid.s.id)
          {
            case PROG_MOCK_CFG_IOBIND:
              {
                const SObjectId::UObjectId *tmp =
                  reinterpret_cast<const SObjectId::UObjectId *>(&item->data);
                this->alloc_object(tmp);
                offset += 2;
                break;
              }

            default:
              {
                DAWNERR("unsupported dummy cfg %d\n", item->cfgid.s.id);
                DAWNASSERT(0, "not supported");
              }
          }
      }
  }

  void alloc_object(const SObjectId::UObjectId *obj)
  {
    this->setObjectMapItem(obj->v, nullptr);
  }
};

//***************************************************************************
// Description: fake program factory creates program mock instances.
//***************************************************************************

class CProgMockFactoryFake : public IProgFactory
{
public:
  CProgMockFactoryFake() {};
  virtual ~CProgMockFactoryFake() override {};

  CProgCommon *create(CDescObject &desc) override
  {
    if (SObjectId::objectIdGetNoId(desc.getObjectIdV()) == CProgMockCommon::CPROGMOCHFACTORY_ID)
      {
        return new CProgMockCommon(desc);
      }

    return nullptr;
  }
};
