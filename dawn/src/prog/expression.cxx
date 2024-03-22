// dawn/src/prog/expression.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/expression.hxx"

#include <climits>
#include <cstring>
#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

static constexpr uint32_t EXPRESSION_VALUE_BITS = sizeof(uint32_t) * CHAR_BIT;

CProgExpression::CProgExpression(CDescObject &desc)
  : CProgCommon(desc)
  , opType(OP_SHIFT_LEFT)
  , constant(0)
  , active(false)
  , registered(false)
{
}

CProgExpression::~CProgExpression()
{
  deinit();
}

int CProgExpression::allocBind(SObjectId::ObjectId sourceId, SObjectId::ObjectId outputId)
{
  SExpressionBind *bind;

  bind = new (std::nothrow) SExpressionBind();
  if (!bind)
    {
      return -ENOMEM;
    }

  bind->owner = this;
  bind->sourceId = sourceId;
  bind->outputId = outputId;
  bind->source = nullptr;
  bind->output = nullptr;

  binds.push_back(bind);
  setObjectMapItem(sourceId, nullptr);
  setObjectMapItem(outputId, nullptr);
  return OK;
}

int CProgExpression::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  const SObjectCfg::ObjectCfgData_t *raw;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_EXPRESSION)
        {
          DAWNERR("expression: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_EXPRESSION_CFG_IOBIND:
            {
              size_t nitems = static_cast<size_t>(item->cfgid.s.size);
              if (nitems == 0 || nitems % 2 != 0)
                {
                  DAWNERR("expression: invalid IOBIND size %zu\n", nitems);
                  return -EINVAL;
                }

              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              for (size_t b = 0; b < nitems / 2; b++)
                {
                  int ret = allocBind(ids[b * 2].v, ids[b * 2 + 1].v);
                  if (ret != OK)
                    {
                      return ret;
                    }
                }
              break;
            }

          case PROG_EXPRESSION_CFG_OP:
            {
              if (item->cfgid.s.size != 2)
                {
                  DAWNERR("expression: OP config must have exactly 2 entries\n");
                  return -EINVAL;
                }

              raw = reinterpret_cast<const SObjectCfg::ObjectCfgData_t *>(item->data);
              opType = SObjectCfg::cfgToU32(raw[0]);
              constant = SObjectCfg::cfgToU32(raw[1]);
              break;
            }

          default:
            {
              DAWNERR("expression: unsupported cfg id %u\n", item->cfgid.s.id);
              return -EINVAL;
            }
        }
    }

  if (binds.empty())
    {
      DAWNERR("expression: at least one IOBIND entry required\n");
      return -EINVAL;
    }

  if (opType > OP_SUB)
    {
      DAWNERR("expression: invalid op type %" PRIu32 "\n", opType);
      return -EINVAL;
    }

  if ((opType == OP_SHIFT_LEFT || opType == OP_SHIFT_RIGHT) && constant >= EXPRESSION_VALUE_BITS)
    {
      DAWNERR("expression: invalid shift constant %" PRIu32 "\n", constant);
      return -EINVAL;
    }

  return OK;
}

int CProgExpression::configure()
{
  return configureDesc(getDesc());
}

int CProgExpression::init()
{
  int ret;

  for (auto bind : binds)
    {
      bind->source = getIO(bind->sourceId);
      bind->output = getIO(bind->outputId);
      if (!bind->source || !bind->output)
        {
          return -EIO;
        }

      ret = prepareWritableTarget(bind->output, 1, true);
      if (ret != OK)
        {
          return ret;
        }

      if (!bind->source->isRead() || bind->source->getDtype() != SObjectId::DTYPE_UINT32 ||
          bind->source->getDataDim() != 1 || !bind->output->isWrite() ||
          bind->output->getDtype() != SObjectId::DTYPE_UINT32 || bind->output->getDataDim() != 1)
        {
          DAWNERR("expression: only scalar uint32 source/output IO is supported\n");
          return -EINVAL;
        }
    }

  return OK;
}

int CProgExpression::deinit()
{
  doStop();

  for (auto bind : binds)
    {
      delete bind;
    }

  binds.clear();
  return OK;
}

bool CProgExpression::compute(uint32_t input, uint32_t &result, const char *phase)
{
  switch (opType)
    {
      case OP_SHIFT_LEFT:
        result = input << constant;
        return true;
      case OP_SHIFT_RIGHT:
        result = input >> constant;
        return true;
      case OP_CONST_LEFT_SHIFT:
        if (input >= EXPRESSION_VALUE_BITS)
          {
            DAWNERR("expression: invalid %s shift %" PRIu32 "\n", phase, input);
            return false;
          }
        result = constant << input;
        return true;
      case OP_ADD:
        result = input + constant;
        return true;
      case OP_SUB:
        result = input - constant;
        return true;
      default:
        return false;
    }
}

void CProgExpression::refresh(SExpressionBind *bind, io_ddata_t *data, const char *phase)
{
  uint32_t input;
  uint32_t result;
  int ret;

  if (data && data->getItems() >= 1)
    {
      std::memcpy(
        bind->sourceData.getDataPtr(), data->getDataPtr(), bind->sourceData.getDataSize());
    }
  else if (bind->source->getData(bind->sourceData, 1) != OK)
    {
      return;
    }

  input = bind->sourceData(0);
  if (!compute(input, result, phase))
    {
      return;
    }

  bind->outputData(0) = result;
  ret = bind->output->setData(bind->outputData);
  if (ret != OK)
    {
      DAWNERR("expression: setData failed %d\n", ret);
    }
}

int CProgExpression::ioNotifierCb(void *priv, io_ddata_t *data)
{
  SExpressionBind *bind = static_cast<SExpressionBind *>(priv);

  if (bind && bind->owner && bind->owner->active)
    {
      bind->owner->refresh(bind, data, "runtime");
    }

  return OK;
}

int CProgExpression::doStart()
{
  int ret;

  if (!registered)
    {
      for (auto bind : binds)
        {
          if (!bind->source->isNotify())
            {
              continue;
            }

          ret = bind->source->setNotifier(ioNotifierCb, 0, bind);
          if (ret != OK)
            {
              return ret;
            }
        }

      registered = true;
    }

  active = true;

  for (auto bind : binds)
    {
      refresh(bind, nullptr, "initial");
    }

  return OK;
}

int CProgExpression::doStop()
{
  if (registered)
    {
      for (auto bind : binds)
        {
          if (bind->source && bind->source->isNotify())
            {
              bind->source->setNotifier(nullptr, 0, nullptr);
            }
        }

      registered = false;
    }

  active = false;
  return OK;
}

bool CProgExpression::hasThread() const
{
  return false;
}
