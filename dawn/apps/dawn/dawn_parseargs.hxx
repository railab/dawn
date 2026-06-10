// dawn/apps/dawn/dawn_parseargs.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"

struct args_s
{
  const char *descPath; ///< Path to descriptor binary file (-f)
};

void parse_args(struct args_s *args, int argc, char **argv);
int validate_args(const struct args_s *args);
