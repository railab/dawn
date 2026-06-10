// dawn/apps/dawn/dawn_parseargs.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn_parseargs.hxx"
#include "dawn_debug.hxx"

#include "dawn/debug.hxx"

#include <cstdlib>
#include <cstring>

static void print_usage(const char *progname)
{
  printf("Usage: %s [options]\n", progname);
  printf("\n");
  printf("Options:\n");
  printf("  -h, --help    Show this help message and exit\n");
#ifdef CONFIG_DAWN_APPS_DESC_SOURCE_FILE
  printf("  -f <path>     Load descriptor from binary file\n");
#endif
}

void parse_args(struct args_s *args, int argc, char **argv)
{
  int i;

  args->descPath = nullptr;

  for (i = 1; i < argc; i++)
    {
      if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0)
        {
          print_usage(argv[0]);
          std::exit(EXIT_SUCCESS);
        }
#ifdef CONFIG_DAWN_APPS_DESC_SOURCE_FILE
      else if (std::strcmp(argv[i], "-f") == 0)
        {
          if (i + 1 < argc)
            {
              args->descPath = argv[++i];
            }
          else
            {
              printf("ERROR: -f requires a path argument\n");
              std::exit(EXIT_FAILURE);
            }
        }
#endif
      else
        {
          printf("ERROR: unknown option '%s'\n", argv[i]);
          print_usage(argv[0]);
          std::exit(EXIT_FAILURE);
        }
    }
}

int validate_args(const struct args_s *args)
{
#if defined(CONFIG_DAWN_APPS_DESC_SOURCE_FILE) && !defined(CONFIG_DAWN_APPS_DESC_SOURCE_BUILTIN)
  if (args->descPath == nullptr)
    {
      DAWNERR("ERROR: -f <path> is required when "
              "FILE is enabled without BUILTIN\n");
      return -EINVAL;
    }
#endif

#ifndef CONFIG_DAWN_APPS_DESC_SOURCE_FILE
  if (args->descPath != nullptr)
    {
      DAWNERR("ERROR: -f is not supported (FILE disabled)\n");
      return -EINVAL;
    }
#endif

  return OK;
}
