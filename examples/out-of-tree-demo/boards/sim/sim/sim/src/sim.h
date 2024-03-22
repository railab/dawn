/*
 * examples/out-of-tree-demo/boards/sim/sim/sim/src/sim.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __DAWN_OOT_DEMO_BOARDS_SIM_SIM_SIM_SRC_SIM_H
#define __DAWN_OOT_DEMO_BOARDS_SIM_SIM_SIM_SRC_SIM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "dawn/fake_drivers.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SIM_PROCFS_MOUNTPOINT "/proc"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int sim_bringup(void);

#endif /* __DAWN_OOT_DEMO_BOARDS_SIM_SIM_SIM_SRC_SIM_H */
