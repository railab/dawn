/*
 * boards/common/include/fakesensor2.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __DAWN_BOARDS_COMMON_INCLUDE_FAKESENSOR2_H
#define __DAWN_BOARDS_COMMON_INCLUDE_FAKESENSOR2_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct fakesensor2_data_s
{
  int              interval;    /* Interval in microseconds */
  size_t           dlen;        /* Data length in data array */
  FAR const float *data;        /* Pointer to data array */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Name: fakesensor2_init
 *
 * Description:
 *   This function generates a sensor node under /dev/uorb/ and reports
 *   data from a provided in-memory samples array.
 *
 * Input Parameters:
 *   type         - The type of sensor defined in <nuttx/sensors/sensor.h>
 *   samples      - Sensor data provided as structure with data array
 *   devno        - The user specifies which device of this type, from 0.
 *   batch_number - The maximum number of batch
 *
 ****************************************************************************/

int fakesensor2_init(int type, FAR const struct fakesensor2_data_s *samples,
                     int devno, uint32_t batch_number);

#ifdef __cplusplus
}
#endif

#endif /* __DAWN_BOARDS_COMMON_INCLUDE_FAKESENSOR2_H */
