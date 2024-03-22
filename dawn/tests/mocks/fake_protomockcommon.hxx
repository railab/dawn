// dawn/tests/mocks/fake_protomockcommon.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "dawn/proto/common.hxx"
#include "dawn/proto/factory.hxx"

using namespace dawn;

//***************************************************************************
// Description: protocol mock tracks start/stop and configured IO bindings.
//***************************************************************************

class CProtoMockCommon : public CProtoCommon
{
public:
  static const uint32_t CPROTOMOCHFACTORY_ID =
    SObjectId::objectId(SObjectId::OBJTYPE_PROTO, 500, 0, 0, 0);

  bool running;

  enum proto_mock_cfg_e
  {
    PROTO_MOCK_CFG_FIRST = 0,
    PROTO_MOCK_CFG_IOBIND = 1,
    PROTO_MOCK_CFG_LAST = 63
  } typedef EProtoMockCfg;

  explicit CProtoMockCommon(CDescObject &desc)
    : CProtoCommon(desc)
  {
    this->running = false;
    this->configure(this->getDesc());
  };

  ~CProtoMockCommon() override {};

  static constexpr SObjectId::ObjectId objectId(uint16_t id)
  {
    return CPROTOMOCHFACTORY_ID + id;
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
            case PROTO_MOCK_CFG_IOBIND:
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
// Description: fake protocol factory creates protocol mock instances.
//***************************************************************************

class CProtoMockFactoryFake : public IProtoFactory
{
public:
  CProtoMockFactoryFake() {};
  virtual ~CProtoMockFactoryFake() override {};

  CProtoCommon *create(CDescObject &cfg) override
  {
    if (SObjectId::objectIdGetNoId(cfg.getObjectIdV()) == CProtoMockCommon::CPROTOMOCHFACTORY_ID)
      {
        return new CProtoMockCommon(cfg);
      }

    return nullptr;
  }
};
