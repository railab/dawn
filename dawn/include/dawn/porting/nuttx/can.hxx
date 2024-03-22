// dawn/include/dawn/porting/nuttx/can.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <nuttx/can/can.h>

// PATH format for CAN

#define CAN_PATH_FMT "/dev/can%d"

// CAN data length

#define CAN_DATA_MAX CAN_MAXDATALEN
