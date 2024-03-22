// dawn/include/dawn/prog/expression.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>
#include <inttypes.h>
#include <vector>

#include "dawn/io/sdata.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
class CIOCommon;
class io_ddata_t;

/**
 * @brief Arithmetic expression evaluator on IO values.
 *
 * Applies a configurable operation to the source value with a constant
 * operand and writes the result to the output IO. Supported operations
 * include shift, add, subtract, and constant-prefix shift.
 */

class CProgExpression : public CProgCommon
{
public:
  enum
  {
    PROG_EXPRESSION_CFG_FIRST = 0,
    PROG_EXPRESSION_CFG_IOBIND = 1,
    PROG_EXPRESSION_CFG_OP = 2,
    PROG_EXPRESSION_CFG_LAST = 31
  };

  enum
  {
    OP_SHIFT_LEFT = 0,
    OP_SHIFT_RIGHT = 1,
    OP_CONST_LEFT_SHIFT = 2,
    OP_ADD = 3,
    OP_SUB = 4,
  };

  explicit CProgExpression(CDescObject &desc);
  ~CProgExpression() override;

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;
  bool hasThread() const override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "expression";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_EXPRESSION, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_EXPRESSION,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgExpression::cfgId(false, size, PROG_EXPRESSION_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdOp(uint16_t size = 2)
  {
    return CProgExpression::cfgId(false, 2, PROG_EXPRESSION_CFG_OP);
  }

private:
  struct SExpressionBind
  {
    CProgExpression *owner;
    SObjectId::ObjectId sourceId;
    SObjectId::ObjectId outputId;
    CIOCommon *source;
    CIOCommon *output;
    io_sdata_t<uint32_t, 1, 1> sourceData;
    io_sdata_t<uint32_t, 1, 1> outputData;
  };

  std::vector<SExpressionBind *> binds; ///< Configured source/output binds.
  uint32_t opType;                      ///< Operation type (OP_* enum).
  uint32_t constant;                    ///< Constant operand.
  bool active;                          ///< Activation flag.
  bool registered;                      ///< Whether source notifiers are registered.

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  int configureDesc(const CDescObject &desc);
  int allocBind(SObjectId::ObjectId sourceId, SObjectId::ObjectId outputId);
  bool compute(uint32_t input, uint32_t &result, const char *phase);
  void refresh(SExpressionBind *bind, io_ddata_t *data, const char *phase);
};
} // Namespace dawn
