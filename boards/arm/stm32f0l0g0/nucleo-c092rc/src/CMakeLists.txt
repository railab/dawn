# boards/arm/stm32f0l0g0/nucleo-c092rc/src/CMakeLists.txt
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

if(CONFIG_BOARDCTL)
  list(APPEND SRCS stm32_appinit.c)
endif()

if(CONFIG_ADC)
  list(APPEND SRCS stm32_adc.c)
endif()

if(CONFIG_STM32F0L0G0_FDCAN)
  if(CONFIG_STM32F0L0G0_FDCAN_CHARDRIVER)
    list(APPEND SRCS stm32_can.c)
  endif()
  if(CONFIG_STM32F0L0G0_FDCAN_SOCKET)
    list(APPEND SRCS stm32_cansock.c)
  endif()
endif()

target_sources(board PRIVATE ${SRCS})

set_property(GLOBAL PROPERTY LD_SCRIPT "${NUTTX_BOARD_DIR}/scripts/flash.ld")
