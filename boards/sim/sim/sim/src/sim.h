/*
 * boards/sim/sim/sim/src/sim.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __DAWN_BOARDS_SIM_SIM_SIM_SRC_SIM_H
#define __DAWN_BOARDS_SIM_SIM_SIM_SRC_SIM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/* Include common fake drivers */

#ifdef CONFIG_DAWN_FAKE_DRIVERS
#  include "dawn/fake_drivers.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* procfs File System */

#define SIM_PROCFS_MOUNTPOINT "/proc"
#define SIM_HOSTFS_MOUNTPOINT "/host"
#define SIM_HOSTFS_CONFIG     "fs=/tmp/dawn"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: sim_bringup
 *
 * Description:
 *   Bring up simulated board features
 *
 ****************************************************************************/

int sim_bringup(void);

#endif /* __DAWN_BOARDS_SIM_SIM_SIM_SRC_SIM_H */
