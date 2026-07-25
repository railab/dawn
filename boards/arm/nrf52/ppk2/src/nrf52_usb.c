/****************************************************************************
 * boards/arm/nrf52/ppk2/src/nrf52_usb.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <nuttx/debug.h>

#include <nuttx/arch.h>
#include <nuttx/usb/usbdev.h>

#include "arm_internal.h"
#include "nrf52_usbd.h"
#include "hardware/nrf52_usbd.h"

#include "ppk2.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  nrf52_usbsuspend
 *
 * Description:
 *   Board logic must provide the nrf52_usbsuspend logic if the USBDEV
 *   driver is used.  This function is called whenever the USB enters or
 *   leaves suspend mode.
 *   This is an opportunity for the board logic to shutdown clocks, power,
 *   etc. while the USB is suspended.
 *
 ****************************************************************************/

void nrf52_usbsuspend(struct usbdev_s *dev, bool resume)
{
  uinfo("resume: %d\n", resume);
}

/****************************************************************************
 * Name: dawn_board_net_ready
 *
 * Description:
 *   Called by Dawn once the network (and dhcpd) is up. A CDC-ECM host runs
 *   DHCP only on the initial USB link-up, which happens at boot before
 *   dhcpd is listening, so it falls back to link-local. Force a USB
 *   re-enumeration by toggling the D+ pull-up: the host re-runs DHCP against
 *   the now-ready server and gets a lease, keeping the gadget plug-and-play.
 *
 ****************************************************************************/

void dawn_board_net_ready(void)
{
  /* Connect the D+ pull-up now. The gadget was kept detached at boot (see
   * nrf52_bringup) until this point, so the host's very first enumeration
   * - and therefore its first and only DHCP attempt - happens with dhcpd
   * already listening. The lease succeeds and the host never falls back to
   * (and gets stuck on) a link-local address.
   */

  putreg32(USBD_USBPULLUP_ENABLE, NRF52_USBD_USBPULLUP);
}

/****************************************************************************
 * Name: ppk2_usb_detach
 *
 * Description:
 *   Disconnect the USB D+ pull-up, keeping the gadget invisible to the host
 *   until dawn_board_net_ready() re-attaches it. Called right after the USB
 *   class is registered so the host does not enumerate before dhcpd is up.
 *
 ****************************************************************************/

void ppk2_usb_detach(void)
{
  putreg32(USBD_USBPULLUP_DISABLE, NRF52_USBD_USBPULLUP);
}
