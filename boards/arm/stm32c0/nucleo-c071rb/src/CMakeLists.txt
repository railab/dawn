# boards/arm/stm32c0/nucleo-c071rb/src/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

set(SRCS stm32_boot.c stm32_bringup.c)

if(CONFIG_ARCH_LEDS)
  list(APPEND SRCS stm32_autoleds.c)
else()
  list(APPEND SRCS stm32_userleds.c)
endif()

if(CONFIG_ARCH_BUTTONS)
  list(APPEND SRCS stm32_buttons.c)
endif()

if(CONFIG_ADC)
  list(APPEND SRCS stm32_adc.c)
endif()

if(CONFIG_PWM)
  list(APPEND SRCS stm32_pwm.c)
endif()

if(CONFIG_PULSECOUNT)
  list(APPEND SRCS stm32_pulsecount.c)
endif()

if(CONFIG_STM32_FDCAN)
  if(CONFIG_STM32_FDCAN_CHARDRIVER)
    list(APPEND SRCS stm32_can.c)
  endif()
  if(CONFIG_STM32_FDCAN_SOCKET)
    list(APPEND SRCS stm32_cansock.c)
  endif()
endif()

target_sources(board PRIVATE ${SRCS})

# Pull stm32 board common from upstream NuttX (boards/arm/common/stm32)

if(CONFIG_ARCH_BOARD_COMMON)
  set(SRCS "")
  add_subdirectory(${NUTTX_DIR}/boards/arm/common/stm32
                   ${CMAKE_BINARY_DIR}/boards/arm/common/stm32)
endif()

set_property(GLOBAL PROPERTY LD_SCRIPT "${NUTTX_BOARD_DIR}/scripts/flash.ld")
