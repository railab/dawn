// dawn/apps/dawn/dawn_parseargs.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn_parseargs.hxx"
#include "dawn_debug.hxx"

#include <cstdlib>
#include <cstring>

static void print_usage(const char *progname)
{
  printf("Usage: %s [options]\n", progname);
  printf("\n");
  printf("Options:\n");
  printf("  -h, --help    Show this help message and exit\n");
}

void parse_args(const struct args_s *args, int argc, char **argv)
{
  int i;

  (void)args;

  for (i = 1; i < argc; i++)
    {
      if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0)
        {
          print_usage(argv[0]);
          std::exit(EXIT_SUCCESS);
        }
    }
}

int validate_args(const struct args_s *args)
{
  (void)args;

  return OK;
}
