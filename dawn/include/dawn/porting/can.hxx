// dawn/include/dawn/porting/can.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/nuttx/can.hxx"

namespace dawn
{
namespace porting
{
/**
 * @brief Common CAN message format for chardev and socketCAN.
 * @ingroup porting
 */

struct canmsg_s
{
  uint32_t id;
  uint8_t len;
  uint8_t rtr : 1;
  uint8_t extid : 1;
  uint8_t error : 1;
  uint8_t _res : 5;
  uint8_t data[CAN_DATA_MAX];
};

} // namespace porting
} // namespace dawn

/**
 * @brief Open CAN device.
 *
 * @param path Device path.
 * @return File descriptor or negative error code.
 */

int can_open(const char *path);

/**
 * @brief Close CAN device.
 *
 * @param fd File descriptor.
 */

void can_close(int fd);

/**
 * @brief Initialize CAN device.
 *
 * @param fd File descriptor.
 * @return OK on success, negative error code on failure.
 */

int can_init(int fd);

/**
 * @brief Read CAN message.
 *
 * @param fd File descriptor.
 * @param msg Pointer to porting::canmsg_s structure to receive message.
 * @return Number of bytes read or negative error code.
 */

int can_read(int fd, FAR dawn::porting::canmsg_s *msg);

/**
 * @brief Send CAN message.
 *
 * @param fd File descriptor.
 * @param msg Pointer to porting::canmsg_s structure to send.
 * @return Number of bytes sent or negative error code.
 */

int can_send(int fd, FAR dawn::porting::canmsg_s *msg);
