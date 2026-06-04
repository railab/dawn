# dawn/src/porting/nuttx/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

set(SRCS board.cxx config.cxx memory.cxx)

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

if(CONFIG_DAWN_IO_PULSECOUNT)
  list(APPEND SRCS pulsecount.cxx)
endif()

if(CONFIG_DAWN_IO_SENSOR OR CONFIG_DAWN_IO_SENSOR_PRODUCER)
  list(APPEND SRCS sensors.cxx)
endif()

if(CONFIG_DAWN_IO_BUTTONS)
  list(APPEND SRCS buttons.cxx)
endif()

if(CONFIG_DAWN_IO_LEDS)
  list(APPEND SRCS leds.cxx)
endif()

if(CONFIG_DAWN_IO_RGB_LED)
  list(APPEND SRCS rgbled.cxx)
endif()

if(CONFIG_DAWN_SYSTEM_LTE)
  list(APPEND SRCS lte.cxx)
endif()

if(CONFIG_DAWN_PROTO_CAN)
  list(APPEND SRCS can.cxx)
endif()

target_sources(dawn_porting PRIVATE ${SRCS})
