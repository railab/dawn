// dawn/tests/test_registry.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "include/test_registry.hxx"

static test_filter_s g_active_filter;
static bool g_has_active_filter = false;
static bool g_case_match_found = false;

static bool starts_with(const char *str, const char *prefix)
{
  size_t prefix_len = strlen(prefix);
  return strncmp(str, prefix, prefix_len) == 0;
}

static void get_module_from_file(const char *file, char *module, size_t module_size)
{
  const char *test_path = strstr(file, "tests/");
  const char *mod_start = nullptr;
  const char *mod_end = nullptr;
  size_t len = 0;

  if (module_size == 0)
    {
      return;
    }

  module[0] = '\0';

  if (test_path == nullptr)
    {
      snprintf(module, module_size, "unknown");
      return;
    }

  mod_start = test_path + strlen("tests/");
  mod_end = strchr(mod_start, '/');
  if (mod_end == nullptr)
    {
      snprintf(module, module_size, "dawn");
      return;
    }

  if (mod_end == mod_start)
    {
      snprintf(module, module_size, "unknown");
      return;
    }

  len = (size_t)(mod_end - mod_start);
  if (len >= module_size)
    {
      len = module_size - 1;
    }

  memcpy(module, mod_start, len);
  module[len] = '\0';
}

static bool args_need_value(int i, int argc, const char *opt)
{
  if (i + 1 >= argc)
    {
      printf("Missing value for option: %s\n", opt);
      return true;
    }

  return false;
}

static const char *normalize_filter_value(const char *value)
{
  if (value == nullptr || value[0] == '\0')
    {
      return nullptr;
    }

  return value;
}

void test_filter_init(test_filter_s *filter)
{
  filter->module = nullptr;
  filter->test = nullptr;
  filter->prefix = nullptr;
  filter->list_only = false;
  filter->help = false;
}

int test_filter_parse_args(int argc, char *argv[], test_filter_s *filter)
{
  int i = 0;

  for (i = 1; i < argc; i += 1)
    {
      if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
          filter->help = true;
          continue;
        }

      if (strcmp(argv[i], "--list") == 0)
        {
          filter->list_only = true;
          continue;
        }

      if (strcmp(argv[i], "--module") == 0)
        {
          if (args_need_value(i, argc, argv[i]))
            {
              return -EINVAL;
            }

          i += 1;
          filter->module = normalize_filter_value(argv[i]);
          continue;
        }

      if (strcmp(argv[i], "--test") == 0)
        {
          if (args_need_value(i, argc, argv[i]))
            {
              return -EINVAL;
            }

          i += 1;
          filter->test = normalize_filter_value(argv[i]);
          continue;
        }

      if (strcmp(argv[i], "--prefix") == 0)
        {
          if (args_need_value(i, argc, argv[i]))
            {
              return -EINVAL;
            }

          i += 1;
          filter->prefix = normalize_filter_value(argv[i]);
          continue;
        }

      printf("Unknown option: %s\n", argv[i]);
      return -EINVAL;
    }

  return 0;
}

bool test_registry_match(const test_registry_entry_s *entry, const test_filter_s *filter)
{
  if (filter->module != nullptr && strcmp(entry->module, filter->module) != 0)
    {
      return false;
    }

  if (filter->test != nullptr && strcmp(entry->name, filter->test) != 0)
    {
      return false;
    }

  if (filter->prefix != nullptr && !starts_with(entry->name, filter->prefix))
    {
      return false;
    }

  return true;
}

void test_registry_set_active_filter(const test_filter_s *filter)
{
  g_active_filter = *filter;
  g_has_active_filter = true;
}

void test_registry_reset_case_matches()
{
  g_case_match_found = false;
}

bool test_registry_has_case_matches()
{
  return g_case_match_found;
}

bool test_registry_should_run_case(const char *file, const char *test_name)
{
  char module[32];
  char fullname[96];
  bool match = true;

  if (!g_has_active_filter)
    {
      return true;
    }

  get_module_from_file(file, module, sizeof(module));
  snprintf(fullname, sizeof(fullname), "%s.%s", module, test_name);

  if (g_active_filter.module != nullptr && strcmp(module, g_active_filter.module) != 0)
    {
      match = false;
    }

  if (match && g_active_filter.test != nullptr && strcmp(fullname, g_active_filter.test) != 0 &&
      strcmp(test_name, g_active_filter.test) != 0)
    {
      match = false;
    }

  if (match && g_active_filter.prefix != nullptr &&
      !starts_with(fullname, g_active_filter.prefix) &&
      !starts_with(test_name, g_active_filter.prefix))
    {
      match = false;
    }

  if (!match)
    {
      return false;
    }

  g_case_match_found = true;

  if (g_active_filter.list_only)
    {
      printf("  %-10s %s\n", module, fullname);
      return false;
    }

  return true;
}

void test_registry_list(const test_registry_entry_s *entries,
                        size_t count,
                        const test_filter_s *filter)
{
  size_t i = 0;

  printf("Available tests:\n");
  for (i = 0; i < count; i += 1)
    {
      if (test_registry_match(&entries[i], filter))
        {
          printf("  %-10s %s\n", entries[i].module, entries[i].name);
        }
    }
}

void test_registry_print_usage(const char *progname)
{
  printf("Usage: %s [options]\n", progname);
  printf("Options:\n");
  printf("  --list             List available module runners\n");
  printf("  --module <name>    Run only one module "
         "(porting/common/dev/io/prog/proto/"
         "dawn)\n");
  printf("  --test <name>      Run one test case by name (module.case or case)\n");
  printf("  --prefix <prefix>  Run test cases matching name prefix\n");
  printf("  -h, --help         Show this help\n");
}
