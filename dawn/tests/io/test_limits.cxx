// dawn/tests/io/test_limits.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/objectcfg.hxx"
#include "dawn/io/limits.hxx"
#include "test_common.hxx"

using namespace dawn;

static int bind_triplet(CIOLimits &lim,
                        uint8_t dtype,
                        size_t words,
                        const uint32_t *mn,
                        const uint32_t *mx,
                        const uint32_t *st)
{
  int ret;

  ret = lim.bind(CIOLimits::CFG_LIMIT_MIN, dtype, words, mn);
  if (ret < 0)
    {
      return ret;
    }

  ret = lim.bind(CIOLimits::CFG_LIMIT_MAX, dtype, words, mx);
  if (ret < 0)
    {
      return ret;
    }

  return lim.bind(CIOLimits::CFG_LIMIT_STEP, dtype, words, st);
}

//***************************************************************************
// Description: Default-constructed CIOLimits accepts any payload.
//***************************************************************************

static void test_io_limits_default_unconfigured()
{
  CIOLimits lim;
  uint32_t v = 1234;

  TEST_ASSERT_FALSE(lim.isConfigured());
  TEST_ASSERT_NULL(lim.getMin());
  TEST_ASSERT_NULL(lim.getMax());
  TEST_ASSERT_NULL(lim.getStep());
  TEST_ASSERT_EQUAL(0, lim.getWords());

  // Unconfigured limits must accept any payload (validate is a no-op).

  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));
}

//***************************************************************************
// Description: bind() rejects null data and zero-words arguments.
//***************************************************************************

static void test_io_limits_bind_invalid_args()
{
  CIOLimits lim;
  uint32_t one = 1;

  TEST_ASSERT_EQUAL(-EINVAL,
                    lim.bind(CIOLimits::CFG_LIMIT_MIN, SObjectId::DTYPE_UINT32, 1, nullptr));
  TEST_ASSERT_EQUAL(-EINVAL, lim.bind(CIOLimits::CFG_LIMIT_MIN, SObjectId::DTYPE_UINT32, 0, &one));
  TEST_ASSERT_FALSE(lim.isConfigured());
}

//***************************************************************************
// Description: bind() rejects an unknown limit id.
//***************************************************************************

static void test_io_limits_bind_unknown_id()
{
  CIOLimits lim;
  uint32_t one = 1;

  TEST_ASSERT_EQUAL(-EINVAL, lim.bind(0xff, SObjectId::DTYPE_UINT32, 1, &one));
}

//***************************************************************************
// Description: bind() rejects mismatched dtype/words across calls.
//***************************************************************************

static void test_io_limits_bind_mismatch()
{
  CIOLimits lim;
  uint32_t mn[2] = {0, 0};
  uint32_t mx[2] = {10, 20};

  // First bind locks dtype=UINT32 and words=2.

  TEST_ASSERT_EQUAL(OK, lim.bind(CIOLimits::CFG_LIMIT_MIN, SObjectId::DTYPE_UINT32, 2, mn));

  // Second bind with a different dtype must be rejected.

  TEST_ASSERT_EQUAL(-EINVAL, lim.bind(CIOLimits::CFG_LIMIT_MAX, SObjectId::DTYPE_INT32, 2, mx));

  // Second bind with a different word count must be rejected.

  TEST_ASSERT_EQUAL(-EINVAL, lim.bind(CIOLimits::CFG_LIMIT_MAX, SObjectId::DTYPE_UINT32, 1, mx));
}

//***************************************************************************
// Description: reset() clears all bindings.
//***************************************************************************

static void test_io_limits_reset()
{
  CIOLimits lim;
  uint32_t mn[1] = {0};
  uint32_t mx[1] = {100};
  uint32_t st[1] = {1};

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_UINT32, 1, mn, mx, st));
  TEST_ASSERT_TRUE(lim.isConfigured());

  lim.reset();

  TEST_ASSERT_FALSE(lim.isConfigured());
  TEST_ASSERT_NULL(lim.getMin());
  TEST_ASSERT_NULL(lim.getMax());
  TEST_ASSERT_NULL(lim.getStep());
  TEST_ASSERT_EQUAL(0, lim.getWords());
}

//***************************************************************************
// Description: validate() with only MIN bound (incomplete) must fail.
//***************************************************************************

static void test_io_limits_validate_incomplete_binding()
{
  CIOLimits lim;
  uint32_t mn = 0;
  uint32_t v = 5;

  TEST_ASSERT_EQUAL(OK, lim.bind(CIOLimits::CFG_LIMIT_MIN, SObjectId::DTYPE_UINT32, 1, &mn));

  // isConfigured() returns true after a single bind, but validate must
  // refuse partial configurations (no MAX, no STEP).

  TEST_ASSERT_TRUE(lim.isConfigured());
  TEST_ASSERT_EQUAL(-EINVAL, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));
}

//***************************************************************************
// Description: validate() rejects dtype/words mismatch on payload.
//***************************************************************************

static void test_io_limits_validate_payload_mismatch()
{
  CIOLimits lim;
  uint32_t mn[1] = {0};
  uint32_t mx[1] = {100};
  uint32_t st[1] = {1};
  uint32_t v[2] = {5, 5};

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_UINT32, 1, mn, mx, st));

  // Wrong dtype.

  TEST_ASSERT_EQUAL(-EINVAL, lim.validate(v, 1, SObjectId::DTYPE_INT32));

  // Wrong word count.

  TEST_ASSERT_EQUAL(-EINVAL, lim.validate(v, 2, SObjectId::DTYPE_UINT32));

  // Null payload.

  TEST_ASSERT_EQUAL(-EINVAL, lim.validate(nullptr, 1, SObjectId::DTYPE_UINT32));
}

//***************************************************************************
// Description: validate() boundary checks on uint32 (min/max inclusive,
// step alignment).
//***************************************************************************

static void test_io_limits_validate_uint32_boundaries()
{
  CIOLimits lim;
  uint32_t mn[1] = {10};
  uint32_t mx[1] = {50};
  uint32_t st[1] = {5};
  uint32_t v;

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_UINT32, 1, mn, mx, st));

  // Inclusive min.

  v = 10;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));

  // Inclusive max.

  v = 50;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));

  // Below min.

  v = 9;
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));

  // Above max.

  v = 51;
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));

  // Step misalignment ((v - min) % step != 0).

  v = 12;
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));

  // Aligned to step from min.

  v = 35;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));
}

//***************************************************************************
// Description: validate() with step==0 disables step alignment check.
//***************************************************************************

static void test_io_limits_validate_step_zero()
{
  CIOLimits lim;
  uint32_t mn[1] = {0};
  uint32_t mx[1] = {100};
  uint32_t st[1] = {0};
  uint32_t v;

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_UINT32, 1, mn, mx, st));

  v = 0;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));

  v = 37; // arbitrary value within range, step alignment bypassed
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));

  v = 100;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));

  v = 101;
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));
}

//***************************************************************************
// Description: validate() with min==max accepts only that single value.
//***************************************************************************

static void test_io_limits_validate_single_value()
{
  CIOLimits lim;
  uint32_t mn[1] = {7};
  uint32_t mx[1] = {7};
  uint32_t st[1] = {1};
  uint32_t v;

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_UINT32, 1, mn, mx, st));

  v = 7;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));

  v = 6;
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));

  v = 8;
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_UINT32));
}

#ifdef CONFIG_DAWN_DTYPE_BOOL

//***************************************************************************
// Description: validate() bool path (treated as unsigned scalar).
//***************************************************************************

static void test_io_limits_validate_bool()
{
  CIOLimits lim;
  uint32_t mn[1] = {0};
  uint32_t mx[1] = {1};
  uint32_t st[1] = {1};
  uint32_t v;

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_BOOL, 1, mn, mx, st));

  v = 0;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_BOOL));

  v = 1;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_BOOL));

  v = 2;
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_BOOL));
}

#endif // CONFIG_DAWN_DTYPE_BOOL

#ifdef CONFIG_DAWN_DTYPE_UINT8

//***************************************************************************
// Description: validate() uint8 path.
//***************************************************************************

static void test_io_limits_validate_uint8()
{
  CIOLimits lim;
  uint32_t mn[1] = {0};
  uint32_t mx[1] = {255};
  uint32_t st[1] = {1};
  uint32_t v;

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_UINT8, 1, mn, mx, st));

  v = 0;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT8));

  v = 128;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT8));

  v = 255;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT8));

  v = 256;
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_UINT8));
}

#endif // CONFIG_DAWN_DTYPE_UINT8

#ifdef CONFIG_DAWN_DTYPE_UINT16

//***************************************************************************
// Description: validate() uint16 path.
//***************************************************************************

static void test_io_limits_validate_uint16()
{
  CIOLimits lim;
  uint32_t mn[1] = {100};
  uint32_t mx[1] = {1000};
  uint32_t st[1] = {100};
  uint32_t v;

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_UINT16, 1, mn, mx, st));

  v = 100;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT16));

  v = 500;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT16));

  v = 1000;
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_UINT16));

  // Misaligned to step.

  v = 150;
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_UINT16));
}

#endif // CONFIG_DAWN_DTYPE_UINT16

#ifdef CONFIG_DAWN_DTYPE_INT16

//***************************************************************************
// Description: validate() int16 path with negative range.
//***************************************************************************

static void test_io_limits_validate_int16()
{
  CIOLimits lim;
  uint32_t mn[1] = {SObjectCfg::i32ToCfg(-100)};
  uint32_t mx[1] = {SObjectCfg::i32ToCfg(100)};
  uint32_t st[1] = {SObjectCfg::i32ToCfg(10)};
  uint32_t v;

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_INT16, 1, mn, mx, st));

  v = SObjectCfg::i32ToCfg(-100);
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_INT16));

  v = SObjectCfg::i32ToCfg(0);
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_INT16));

  v = SObjectCfg::i32ToCfg(100);
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_INT16));

  v = SObjectCfg::i32ToCfg(-101);
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_INT16));

  v = SObjectCfg::i32ToCfg(101);
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_INT16));

  // Step misaligned.

  v = SObjectCfg::i32ToCfg(-95);
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_INT16));
}

#endif // CONFIG_DAWN_DTYPE_INT16

#ifdef CONFIG_DAWN_DTYPE_INT32

//***************************************************************************
// Description: validate() int32 path.
//***************************************************************************

static void test_io_limits_validate_int32()
{
  CIOLimits lim;
  uint32_t mn[1] = {SObjectCfg::i32ToCfg(-1000000)};
  uint32_t mx[1] = {SObjectCfg::i32ToCfg(1000000)};
  uint32_t st[1] = {SObjectCfg::i32ToCfg(0)};
  uint32_t v;

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_INT32, 1, mn, mx, st));

  v = SObjectCfg::i32ToCfg(-1000000);
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_INT32));

  v = SObjectCfg::i32ToCfg(0);
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_INT32));

  v = SObjectCfg::i32ToCfg(1000000);
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_INT32));

  v = SObjectCfg::i32ToCfg(-1000001);
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_INT32));

  v = SObjectCfg::i32ToCfg(1000001);
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_INT32));
}

#endif // CONFIG_DAWN_DTYPE_INT32

#ifdef CONFIG_DAWN_DTYPE_FLOAT

//***************************************************************************
// Description: validate() float path with epsilon-tolerant step check.
//***************************************************************************

static void test_io_limits_validate_float()
{
  CIOLimits lim;
  uint32_t mn[1] = {SObjectCfg::fToCfg(-1.0f)};
  uint32_t mx[1] = {SObjectCfg::fToCfg(1.0f)};
  uint32_t st[1] = {SObjectCfg::fToCfg(0.25f)};
  uint32_t v;

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_FLOAT, 1, mn, mx, st));

  v = SObjectCfg::fToCfg(-1.0f);
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_FLOAT));

  v = SObjectCfg::fToCfg(0.0f);
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_FLOAT));

  v = SObjectCfg::fToCfg(0.75f);
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_FLOAT));

  v = SObjectCfg::fToCfg(1.0f);
  TEST_ASSERT_EQUAL(OK, lim.validate(&v, 1, SObjectId::DTYPE_FLOAT));

  // Out-of-range.

  v = SObjectCfg::fToCfg(-1.01f);
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_FLOAT));

  v = SObjectCfg::fToCfg(1.01f);
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_FLOAT));

  // Mid-step misalignment.

  v = SObjectCfg::fToCfg(0.10f);
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(&v, 1, SObjectId::DTYPE_FLOAT));
}

#endif // CONFIG_DAWN_DTYPE_FLOAT

//***************************************************************************
// Description: validate() multi-element fails on first out-of-range item.
//***************************************************************************

static void test_io_limits_validate_multidim_first_fail()
{
  CIOLimits lim;
  uint32_t mn[3] = {0, 0, 0};
  uint32_t mx[3] = {10, 20, 30};
  uint32_t st[3] = {1, 1, 1};
  uint32_t v[3];

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_UINT32, 3, mn, mx, st));

  v[0] = 5;
  v[1] = 15;
  v[2] = 25;
  TEST_ASSERT_EQUAL(OK, lim.validate(v, 3, SObjectId::DTYPE_UINT32));

  // First element out of range.

  v[0] = 11;
  v[1] = 15;
  v[2] = 25;
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(v, 3, SObjectId::DTYPE_UINT32));

  // Last element out of range.

  v[0] = 5;
  v[1] = 15;
  v[2] = 31;
  TEST_ASSERT_EQUAL(-ERANGE, lim.validate(v, 3, SObjectId::DTYPE_UINT32));
}

//***************************************************************************
// Description: validate() with unsupported dtype returns -ENOTSUP.
//***************************************************************************

static void test_io_limits_validate_unsupported_dtype()
{
  CIOLimits lim;
  uint32_t mn[1] = {0};
  uint32_t mx[1] = {0};
  uint32_t st[1] = {0};
  uint32_t v = 0;

  // INT64/UINT64/DOUBLE are not handled by the validator switch; bind
  // succeeds (it stores by ID), but validate returns -ENOTSUP.

  TEST_ASSERT_EQUAL(OK, bind_triplet(lim, SObjectId::DTYPE_INT64, 1, mn, mx, st));
  TEST_ASSERT_EQUAL(-ENOTSUP, lim.validate(&v, 1, SObjectId::DTYPE_INT64));
}

extern "C"
{
  int test_io_limits()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_limits_default_unconfigured);
    DAWN_RUN_TEST(test_io_limits_bind_invalid_args);
    DAWN_RUN_TEST(test_io_limits_bind_unknown_id);
    DAWN_RUN_TEST(test_io_limits_bind_mismatch);
    DAWN_RUN_TEST(test_io_limits_reset);
    DAWN_RUN_TEST(test_io_limits_validate_incomplete_binding);
    DAWN_RUN_TEST(test_io_limits_validate_payload_mismatch);
    DAWN_RUN_TEST(test_io_limits_validate_uint32_boundaries);
    DAWN_RUN_TEST(test_io_limits_validate_step_zero);
    DAWN_RUN_TEST(test_io_limits_validate_single_value);
#ifdef CONFIG_DAWN_DTYPE_BOOL
    DAWN_RUN_TEST(test_io_limits_validate_bool);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
    DAWN_RUN_TEST(test_io_limits_validate_uint8);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
    DAWN_RUN_TEST(test_io_limits_validate_uint16);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
    DAWN_RUN_TEST(test_io_limits_validate_int16);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
    DAWN_RUN_TEST(test_io_limits_validate_int32);
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
    DAWN_RUN_TEST(test_io_limits_validate_float);
#endif
    DAWN_RUN_TEST(test_io_limits_validate_multidim_first_fail);
    DAWN_RUN_TEST(test_io_limits_validate_unsupported_dtype);

    return UNITY_END();
  }
}
