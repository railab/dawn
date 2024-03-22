// dawn/include/dawn/porting/buttons.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>

#include "dawn/porting/nuttx/buttons.hxx"

int buttons_open(const char *path);
void buttons_close(int fd);
int buttons_read(int fd, uint32_t *buttons);
