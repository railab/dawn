/*
 * boards/common/include/dawn/fake_files.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __BOARDS_COMMON_INCLUDE_DAWN_FAKE_FILES_H
#define __BOARDS_COMMON_INCLUDE_DAWN_FAKE_FILES_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: dawn_fake_files_init
 *
 * Description:
 *   Pre-populate the configured tmpfs mount point with the set of fake
 *   files selected via Kconfig (CONFIG_DAWN_FAKE_FILE_*). Each enabled
 *   file is created (overwriting any existing content) and seeded with
 *   the canonical "initial data" string.
 *
 *   The mount point is taken from CONFIG_DAWN_FAKE_FILES_MOUNTPOINT and
 *   must already be mounted before this function is called.
 *
 * Returned Value:
 *   Zero on success; a negative errno on failure of any individual file
 *   creation. The function attempts every selected file even if an
 *   earlier one fails.
 *
 ****************************************************************************/

int dawn_fake_files_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __BOARDS_COMMON_INCLUDE_DAWN_FAKE_FILES_H */
