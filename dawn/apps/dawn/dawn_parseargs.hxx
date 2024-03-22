// dawn/apps/dawn/dawn_parseargs.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

struct args_s
{
  int reserved;
};

void parse_args(const struct args_s *args, int argc, char **argv);
int validate_args(const struct args_s *args);
