# ##############################################################################
# boards/arm/nrf91/thingy91/src/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

set(SRCS nrf91_boot.c nrf91_bringup.c)

if(CONFIG_ARCH_BUTTONS)
  list(APPEND SRCS nrf91_buttons.c)
endif()

if(CONFIG_NRF91_MODEM)
  list(APPEND SRCS nrf91_modem.c)
endif()

if(CONFIG_NRF91_I2C2_MASTER)
  list(APPEND SRCS nrf91_pmic.c)
endif()

if(CONFIG_RGBLED)
  list(APPEND SRCS nrf91_rgbled.c)
endif()

if(CONFIG_I2C_EE_24XX)
  list(APPEND SRCS nrf91_eeprom.c)
endif()

if(CONFIG_SENSORS_BME680
   OR CONFIG_SENSORS_BH1749NUC
   OR CONFIG_SENSORS_ADXL362
   OR CONFIG_SENSORS_ADXL372)
  list(APPEND SRCS nrf91_sensors.c)
endif()

target_sources(board PRIVATE ${SRCS})

# Pull chip-common from upstream NuttX (boards/arm/nrf91/common)

if(CONFIG_ARCH_BOARD_COMMON)
  target_include_directories(board PRIVATE ${NUTTX_COMMON_DIR}/include)
  set(SRCS "")
  add_subdirectory(${NUTTX_COMMON_DIR}/src
                   ${CMAKE_BINARY_DIR}/boards/arm/nrf91/common-src)
endif()

if(CONFIG_ARCH_BOARD_COMMON)
  set_property(GLOBAL PROPERTY LD_SCRIPT
                               "${NUTTX_COMMON_DIR}/scripts/flash_app.ld")
else()
  set_property(GLOBAL PROPERTY LD_SCRIPT
                               "${NUTTX_BOARD_DIR}/scripts/flash_app.ld")
endif()
