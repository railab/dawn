// dawn/apps/dawn/dawn_file_load.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Load a binary descriptor from file into a dynamically allocated
 *        buffer.
 *
 * Opens ``path``, determines the file size, validates that it does not
 * exceed ``maxSize``, reads the entire file, and validates the binary
 * descriptor format (magic, CRC, dtypes) through
 * CDescriptor::binValid().
 *
 * @param path    File system path to the binary descriptor file.
 * @param maxSize Maximum allowed file size in bytes.
 * @param outBin  Output pointer set to the allocated buffer on success.
 * @param outLen  Output length set to the file size on success.
 * @return 0 on success, negative errno on failure.
 *
 * On success the caller owns ``*outBin`` and must free it with
 * ``delete[]`` after CDawn has finished with the descriptor.
 */
int dawn_file_load_descriptor(const char *path, size_t maxSize, uint32_t **outBin, size_t *outLen);
