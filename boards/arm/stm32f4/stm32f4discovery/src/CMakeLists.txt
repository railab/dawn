# boards/arm/stm32f4/stm32f4discovery/src/CMakeLists.txt
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

if(CONFIG_STM32_OTGFS)
  list(APPEND SRCS stm32_usb.c)
endif()

if(CONFIG_STM32_SPI1)
  list(APPEND SRCS stm32_spi.c)
endif()

if(CONFIG_SENSORS_LIS3DSH_UORB)
  list(APPEND SRCS stm32_lis3dsh.c)
endif()

target_sources(board PRIVATE ${SRCS})

set_property(GLOBAL PROPERTY LD_SCRIPT "${NUTTX_BOARD_DIR}/scripts/flash.ld")
