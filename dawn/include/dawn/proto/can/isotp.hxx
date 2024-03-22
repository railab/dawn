// dawn/include/dawn/proto/can/isotp.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>
#include <cstdint>

namespace dawn
{
namespace porting
{
struct canmsg_s;
} // namespace porting
} // namespace dawn

namespace dawn
{
/**
 * @brief ISO-TP (ISO 15765-2) transport protocol helper.
 *
 * Minimal ISO-TP implementation for CAN segmented transfers.
 * Uses standard ISO-TP frame format but without flow control.
 * Relies on CAN hardware reliability instead of flow control.
 *
 * This class will be replaced by NuttX ISO-TP implementation in the future.
 */

class CIsoTp
{
public:
  // ISO-TP frame type masks.
  static constexpr uint8_t FRAME_TYPE_MASK = 0xF0;
  static constexpr uint8_t FRAME_SF = 0x00; /**< Single Frame (0x0N) */
  static constexpr uint8_t FRAME_FF = 0x10; /**< First Frame (0x1N) */
  static constexpr uint8_t FRAME_CF = 0x20; /**< Consecutive Frame (0x2N) */
  static constexpr uint8_t FRAME_FC = 0x30; /**< Flow Control (0x3N) */

  // ISO-TP sequence number mask and limit.

  static constexpr uint8_t SEQ_MASK = 0x0F;
  static constexpr uint8_t SEQ_MAX = 0x10;

  struct State
  {
    size_t offset;      ///< Current byte offset in data buffer.
    uint8_t seq_next;   ///< Expected sequence number (0-15).
    size_t total_len;   ///< Total data length from FF.
    bool active;        ///< Transfer in progress.
    uint64_t timestamp; ///< Last frame timestamp (for timeout).
  };

  static void initState(State &state);
  static void resetState(State &state);
  static uint8_t getFrameType(uint8_t pci);
  static uint8_t getSequence(uint8_t pci);
  static uint8_t nextSequence(uint8_t seq);

  static int handleFirstFrame(const porting::canmsg_s &msg,
                              State &state,
                              void *data_buf,
                              size_t buf_size);

  static int handleConsecutiveFrame(const porting::canmsg_s &msg,
                                    State &state,
                                    void *data_buf,
                                    size_t buf_size);

  static bool isComplete(const State &state);
  static bool isTimeout(const State &state, uint64_t now, uint64_t timeout);

  /**
   * @brief Process incoming ISO-TP frame. Handles FF/CF, manages state and
   *        timeouts. Returns -ENOMSG if frame is not an ISO-TP FF/CF.
   * @return 0 on success (in progress), 1 on complete, negative error.
   */

  static int receive(const porting::canmsg_s &msg,
                     State &state,
                     void *data_buf,
                     size_t buf_size,
                     uint64_t now,
                     uint64_t timeout);

  /**
   * @brief Process incoming Custom Segmented frame (Dawn specific). Used for
   *        INDEXED_WRITE and WRITE_ALL segmented transfers.
   * @return 0 on success (in progress), 1 on complete, negative error.
   */

  static int receiveCustom(const porting::canmsg_s &msg,
                           State &state,
                           void *data_buf,
                           size_t buf_size,
                           uint8_t seg_no,
                           bool seg_last,
                           uint64_t now,
                           uint64_t timeout);
};

} // namespace dawn
