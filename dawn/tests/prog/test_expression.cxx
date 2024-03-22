// dawn/tests/prog/test_expression.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/expression.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto EX_SRC = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 90);
static constexpr auto EX_DST = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 91);
static constexpr auto EX_SRC64 = CIOVirt::objectId(SObjectId::DTYPE_UINT64, false, 92);
static constexpr auto EX_DST64 = CIOVirt::objectId(SObjectId::DTYPE_UINT64, false, 93);

static uint32_t g_s[] = {
  EX_SRC,
  0,
};

static uint32_t g_d[] = {
  EX_DST,
  0,
};

static uint32_t g_s64[] = {
  EX_SRC64,
  0,
};

static uint32_t g_d64[] = {
  EX_DST64,
  0,
};

static uint32_t g_shl[] = {
  CProgExpression::objectId(0),
  2,
  CProgExpression::cfgIdIOBind(2),
  EX_SRC,
  EX_DST,
  CProgExpression::cfgIdOp(),
  CProgExpression::OP_SHIFT_LEFT,
  2,
};

static uint32_t g_cshl[] = {
  CProgExpression::objectId(1),
  2,
  CProgExpression::cfgIdIOBind(2),
  EX_SRC,
  EX_DST,
  CProgExpression::cfgIdOp(),
  CProgExpression::OP_CONST_LEFT_SHIFT,
  1,
};

static uint32_t g_add[] = {
  CProgExpression::objectId(2),
  2,
  CProgExpression::cfgIdIOBind(2),
  EX_SRC,
  EX_DST,
  CProgExpression::cfgIdOp(),
  CProgExpression::OP_ADD,
  10,
};

static uint32_t g_bad_shift[] = {
  CProgExpression::objectId(3),
  2,
  CProgExpression::cfgIdIOBind(2),
  EX_SRC,
  EX_DST,
  CProgExpression::cfgIdOp(),
  CProgExpression::OP_SHIFT_LEFT,
  32,
};

static uint32_t g_u64_bind[] = {
  CProgExpression::objectId(4),
  2,
  CProgExpression::cfgIdIOBind(2),
  EX_SRC64,
  EX_DST64,
  CProgExpression::cfgIdOp(),
  CProgExpression::OP_SHIFT_LEFT,
  2,
};

//***************************************************************************
// Description: shift-left expression writes the shifted source value.
//***************************************************************************

static void test_expression_shift_left()
{
  CDescObject sd(g_s);
  CIOVirt s(sd);
  CDescObject dd(g_d);
  CIOVirt d(dd);
  CDescObject pd(g_shl);
  CProgExpression p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(EX_SRC, &s);
  p.setObjectMapItem(EX_DST, &d);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());
  in(0) = 3;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(12, out(0));
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: constant-left-shift expression shifts a constant by the source.
//***************************************************************************

static void test_expression_const_left_shift()
{
  CDescObject sd(g_s);
  CIOVirt s(sd);
  CDescObject dd(g_d);
  CIOVirt d(dd);
  CDescObject pd(g_cshl);
  CProgExpression p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(EX_SRC, &s);
  p.setObjectMapItem(EX_DST, &d);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));
  in(0) = 3;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(8, out(0));
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: add expression adds the configured constant to the source.
//***************************************************************************

static void test_expression_add()
{
  CDescObject sd(g_s);
  CIOVirt s(sd);
  CDescObject dd(g_d);
  CIOVirt d(dd);
  CDescObject pd(g_add);
  CProgExpression p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(EX_SRC, &s);
  p.setObjectMapItem(EX_DST, &d);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());
  in(0) = 5;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(15, out(0));
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: invalid shift constants are rejected during configure.
//***************************************************************************

static void test_expression_rejects_invalid_shift_constant()
{
  CDescObject pd(g_bad_shift);
  CProgExpression p(pd);

  TEST_ASSERT_EQUAL(-EINVAL, p.configure());
}

//***************************************************************************
// Description: expression init rejects non-32-bit IO bindings.
//***************************************************************************

static void test_expression_rejects_non_32bit_io()
{
  CDescObject sd(g_s64);
  CIOVirt s(sd);
  CDescObject dd(g_d64);
  CIOVirt d(dd);
  CDescObject pd(g_u64_bind);
  CProgExpression p(pd);

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(EX_SRC64, &s);
  p.setObjectMapItem(EX_DST64, &d);
  TEST_ASSERT_EQUAL(-EINVAL, p.init());
}

extern "C"
{
  int test_prog_expression()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_expression_shift_left);
    DAWN_RUN_TEST(test_expression_const_left_shift);
    DAWN_RUN_TEST(test_expression_add);
    DAWN_RUN_TEST(test_expression_rejects_invalid_shift_constant);
    DAWN_RUN_TEST(test_expression_rejects_non_32bit_io);
    return UNITY_END();
  }
}
