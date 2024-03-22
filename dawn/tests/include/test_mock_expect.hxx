// dawn/tests/include/test_mock_expect.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>
#include <cstdint>

struct MockCall
{
  uint8_t event;
  int arg0;
  int arg1;
};

enum mock_event_e : uint8_t
{
  MOCK_EVENT_CREATE = 0,
};

struct MockTrace
{
  static const size_t MAX_CALLS = 16;
  MockCall calls[MAX_CALLS];
  size_t count;
};

inline void mock_trace_reset(MockTrace *trace)
{
  trace->count = 0;
}

inline void mock_trace_call(MockTrace *trace, mock_event_e event, int arg0, int arg1)
{
  if (trace == nullptr || trace->count >= MockTrace::MAX_CALLS)
    {
      return;
    }

  trace->calls[trace->count] = {event, arg0, arg1};
  trace->count += 1;
}

inline size_t mock_trace_count(const MockTrace *trace, mock_event_e event)
{
  size_t i = 0;
  size_t count = 0;

  if (trace == nullptr)
    {
      return 0;
    }

  for (i = 0; i < trace->count; i += 1)
    {
      if (trace->calls[i].event == event)
        {
          count += 1;
        }
    }

  return count;
}

inline bool mock_trace_has_order(const MockTrace *trace,
                                 const mock_event_e *expected,
                                 size_t expected_len)
{
  size_t i = 0;
  size_t idx = 0;

  if (expected_len == 0)
    {
      return true;
    }

  if (trace == nullptr || expected == nullptr)
    {
      return false;
    }

  for (i = 0; i < trace->count; i += 1)
    {
      if (trace->calls[i].event == expected[idx])
        {
          idx += 1;
          if (idx == expected_len)
            {
              return true;
            }
        }
    }

  return false;
}

inline bool mock_trace_call_matches(const MockTrace *trace,
                                    size_t index,
                                    mock_event_e event,
                                    int arg0)
{
  if (trace == nullptr || index >= trace->count)
    {
      return false;
    }

  if (trace->calls[index].event != event)
    {
      return false;
    }

  return trace->calls[index].arg0 == arg0;
}

#define MOCK_TRACE_CALL(trace, event, a, b) mock_trace_call((trace), (event), (a), (b))

#define ASSERT_CALLS(trace, event, expected_count)                         \
  TEST_ASSERT_EQUAL((expected_count), mock_trace_count(&(trace), (event)))

#define ASSERT_CALL_ORDER(trace, expected_events)                                         \
  TEST_ASSERT_TRUE(mock_trace_has_order(                                                  \
    &(trace), (expected_events), sizeof(expected_events) / sizeof((expected_events)[0])))

#define ASSERT_CALL_AT(trace, index, event, expected_arg0)                               \
  TEST_ASSERT_TRUE(mock_trace_call_matches(&(trace), (index), (event), (expected_arg0)))
