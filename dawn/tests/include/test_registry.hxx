// dawn/tests/include/test_registry.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>

struct test_registry_entry_s
{
  const char *name;
  const char *module;
  int (*func)();
};

struct test_filter_s
{
  const char *module;
  const char *test;
  const char *prefix;
  bool list_only;
  bool help;
};

void test_filter_init(test_filter_s *filter);
int test_filter_parse_args(int argc, char *argv[], test_filter_s *filter);
bool test_registry_match(const test_registry_entry_s *entry, const test_filter_s *filter);
void test_registry_set_active_filter(const test_filter_s *filter);
void test_registry_reset_case_matches();
bool test_registry_has_case_matches();
bool test_registry_should_run_case(const char *file, const char *test_name);
void test_registry_list(const test_registry_entry_s *entries,
                        size_t count,
                        const test_filter_s *filter);
void test_registry_print_usage(const char *progname);
