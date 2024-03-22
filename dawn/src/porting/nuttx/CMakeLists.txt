# dawn/src/porting/nuttx/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

set(SRCS board.cxx config.cxx)

if(CONFIG_DAWN_IO_ADC)
  list(APPEND SRCS adc.cxx)
endif()

if(CONFIG_DAWN_IO_DAC)
  list(APPEND SRCS dac.cxx)
endif()

if(CONFIG_DAWN_IO_ENCODER OR CONFIG_DAWN_IO_ENCODER_INDEX)
  list(APPEND SRCS encoder.cxx)
endif()

if(CONFIG_DAWN_IO_GPI OR CONFIG_DAWN_IO_GPO)
  list(APPEND SRCS gpio.cxx)
endif()

if(CONFIG_DAWN_IO_PWM)
  list(APPEND SRCS pwm.cxx)
endif()

if(CONFIG_DAWN_IO_SENSOR)
  list(APPEND SRCS sensors.cxx)
endif()

if(CONFIG_DAWN_IO_BUTTONS)
  list(APPEND SRCS buttons.cxx)
endif()

if(CONFIG_DAWN_IO_LEDS)
  list(APPEND SRCS leds.cxx)
endif()

if(CONFIG_DAWN_PROTO_CAN)
  list(APPEND SRCS can.cxx)
endif()

target_sources(dawn_porting PRIVATE ${SRCS})
