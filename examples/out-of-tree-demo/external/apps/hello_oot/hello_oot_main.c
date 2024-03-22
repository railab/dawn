/*
 * examples/out-of-tree-demo/external/apps/hello_oot/hello_oot_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tiny NuttX application defined entirely in an out-of-tree project.
 * Registered via nuttx_add_application() from
 * external/apps/hello_oot/CMakeLists.txt; surfaces as the `hello_oot`
 * NSH builtin.
 */

#include <nuttx/config.h>

#include <stdio.h>

int main(int argc, char *argv[])
{
  printf("hello from the dawn out-of-tree demo (%s)\n",
         CONFIG_LIBC_HOSTNAME);
  return 0;
}
