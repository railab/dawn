// dawn/include/dawn/porting/nuttx/adc.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

// PATH format for ADC

#define ADC_PATH_FMT "/dev/adc%d"

#ifndef ANIOC_STOP
#  define ANIOC_STOP 0x1001
#endif

#ifndef ANIOC_SET_TIMER_FREQ
#  define ANIOC_SET_TIMER_FREQ 0x1002
#endif

#ifndef ANIOC_SAMPLES_ON_READ
#  define ANIOC_SAMPLES_ON_READ 0x1003
#endif
