/*
 * boards/common/include/fake_drivers.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __DAWN_BOARDS_COMMON_INCLUDE_FAKE_DRIVERS_H
#define __DAWN_BOARDS_COMMON_INCLUDE_FAKE_DRIVERS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Fake ADC driver */

#ifdef CONFIG_DAWN_FAKE_ADC
int fake_adc_initialize(int devno);
#endif

/* Fake DAC driver */

#ifdef CONFIG_DAWN_FAKE_DAC
int fake_dac_initialize(int devno);
#endif

/* Fake PWM driver */

#ifdef CONFIG_DAWN_FAKE_PWM
int fake_pwm_initialize(int devno);
#endif

/* Fake encoder driver */

#ifdef CONFIG_DAWN_FAKE_ENCODER
int fake_encoder_initialize(int devno);
#endif

/* Fake GPIO I/O Expander driver */

#ifdef CONFIG_DAWN_FAKE_IOEXPANDER
int fake_ioexpander_initialize(void);
#endif

/* Fake sensor driver */

#ifdef CONFIG_DAWN_FAKE_SENSORS
int fake_sensors_initialize(void);
#endif

/* Fake user LED driver */

#ifdef CONFIG_DAWN_FAKE_USERLEDS
int fake_userleds_initialize(int devno, uint8_t lednum);
#endif

/* Fake button driver */

#ifdef CONFIG_DAWN_FAKE_BUTTONS
int fake_buttons_initialize(int devno, uint8_t btnnum);
#endif

/* Fake unique ID support */

#ifdef CONFIG_DAWN_FAKE_UID
int fake_uid_initialize(void);
#endif

/* Fake reset support */

#ifdef CONFIG_DAWN_FAKE_RESET
void fake_reset_initialize(void);
#endif

/* Fake poweroff support */

#ifdef CONFIG_DAWN_FAKE_POWEROFF
void fake_poweroff_initialize(void);
#endif

#endif /* __DAWN_BOARDS_COMMON_INCLUDE_FAKE_DRIVERS_H */
