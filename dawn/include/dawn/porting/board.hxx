// dawn/include/dawn/porting/board.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

/**
 * @brief Initialize board-specific hardware.
 *
 * @return OK on success, negative error code on failure.
 */

int dawn_board_init();
