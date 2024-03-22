// dawn/tests/prog/test_expression.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/expression.hxx"
#include "test_common.hxx"

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"

using namespace dawn;

static constexpr auto EXPR_SRC = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 90);
static constexpr auto EXPR_DST = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 91);

static uint32_t g_cfg_src[] = {EXPR_SRC, 0};
static uint32_t g_cfg_dst[] = {EXPR_DST, 0};

static uint32_t g_bin_expr_shl[] = {
  CProgExpression::objectId(0),
  2,
  CProgExpression::cfgIdIOBind(2),
  EXPR_SRC,
  EXPR_DST,
  CProgExpression::cfgIdOp(),
  CProgExpression::OP_SHIFT_LEFT,
  2, // constant
};

static uint32_t g_bin_expr_const_shl[] = {
  CProgExpression::objectId(1),
  2,
  CProgExpression::cfgIdIOBind(2),
  EXPR_SRC,
  EXPR_DST,
  CProgExpression::cfgIdOp(),
  CProgExpression::OP_CONST_LEFT_SHIFT,
  1, // constant=1 → 1 << input
};

static uint32_t g_bin_expr_add[] = {
  CProgExpression::objectId(2),
  2,
  CProgExpression::cfgIdIOBind(2),
  EXPR_SRC,
  EXPR_DST,
  CProgExpression::cfgIdOp(),
  CProgExpression::OP_ADD,
  10,
};

static void test_expression_shift_left()
{
  CDescObject srcDesc(g_cfg_src);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_dst);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_expr_shl);
  CProgExpression prog(progDesc);

  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(EXPR_SRC, &src);
  prog.setObjectMapItem(EXPR_DST, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 3;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(12, out(0)); // 3 << 2 = 12

  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

static void test_expression_const_left_shift()
{
  CDescObject srcDesc(g_cfg_src);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_dst);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_expr_const_shl);
  CProgExpression prog(progDesc);

  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(EXPR_SRC, &src);
  prog.setObjectMapItem(EXPR_DST, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0)); // 1 << 0 = 1

  in(0) = 3;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(8, out(0)); // 1 << 3 = 8

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

static void test_expression_add()
{
  CDescObject srcDesc(g_cfg_src);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_dst);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_expr_add);
  CProgExpression prog(progDesc);

  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(EXPR_SRC, &src);
  prog.setObjectMapItem(EXPR_DST, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 5;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(15, out(0)); // 5 + 10

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_expression()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_expression_shift_left);
    DAWN_RUN_TEST(test_expression_const_left_shift);
    DAWN_RUN_TEST(test_expression_add);
    return UNITY_END();
  }
}
