// dawn/src/porting/nuttx/can.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/can.hxx"

#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"

//***************************************************************************
// Public Functions
//***************************************************************************

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: can_open
 *
 * Description:
 *   Open CAN character device interface.
 *
 ****************************************************************************/

int can_open(const char *path)
{
  return open(path, O_RDWR);
}

/****************************************************************************
 * Name: can_close
 *
 * Description:
 *   Close CAN character device interface.
 *
 ****************************************************************************/

void can_close(int fd)
{
  close(fd);
}

/****************************************************************************
 * Name: can_init
 *
 * Description:
 *   Initialzie CAN character device interface.
 *
 ****************************************************************************/

int can_init(int fd)
{
  return OK;
}

/****************************************************************************
 * Name: can_read
 *
 * Description:
 *   Read CAN data (blocking).
 *
 ****************************************************************************/

int can_read(int fd, FAR dawn::porting::canmsg_s *msg)
{
  struct can_msg_s frame;
  ssize_t ret;

  /* Read frame */

  ret = read(fd, &frame, sizeof(struct can_msg_s));
  if (ret < 0)
    {
      if (errno != EAGAIN)
        {
          DAWNERR("read failed %d\n", -errno);
        }

      return -errno;
    }

  /* Convert frame to common format */

  msg->id = frame.cm_hdr.ch_id;
  msg->len = can_dlc2bytes(frame.cm_hdr.ch_dlc);
  msg->rtr = frame.cm_hdr.ch_rtr;
  msg->_res = 0;
#ifdef CONFIG_CAN_EXTID
  msg->extid = frame.cm_hdr.ch_extid;
#else
  msg->extid = 0;
#endif

  std::memcpy(msg->data, frame.cm_data, CAN_DATA_MAX);

  return static_cast<int>(ret);
}

/****************************************************************************
 * Name: can_send
 *
 * Description:
 *   Send CAN data.
 *
 ****************************************************************************/

int can_send(int fd, FAR dawn::porting::canmsg_s *msg)
{
  struct can_msg_s frame;
  ssize_t ret;

  /* Convert from common format */

  frame.cm_hdr.ch_id = msg->id;
  frame.cm_hdr.ch_rtr = msg->rtr;
  frame.cm_hdr.ch_dlc = can_bytes2dlc(msg->len);
#ifdef CONFIG_CAN_ERRORS
  frame.cm_hdr.ch_error = 0;
#endif
#ifdef CONFIG_CAN_EXTID
  frame.cm_hdr.ch_extid = msg->extid;
#endif
  frame.cm_hdr.ch_tcf = 0;
  std::memcpy(frame.cm_data, msg->data, CAN_DATA_MAX);

  /* Send frame */

  ret = write(fd, &frame, CAN_MSGLEN(msg->len));
  if (ret < 0)
    {
      DAWNERR("write failed %d\n", -errno);
      return -errno;
    }

  return static_cast<int>(ret);
}
