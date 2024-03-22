// dawn/include/dawn/io/capabilities.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Read-only seekable capability bitmap IO.
 *
 * Exposes all capability sections in a single binary blob.
 */

class CIOCapabilities : public CIOCommon
{
public:
  enum
  {
    CAPS_BUILD_OS_NUTTX = (1u << 0),
    CAPS_BUILD_FILESYSTEM = (1u << 1),
    CAPS_BUILD_IO_NOTIFY = (1u << 2),
    CAPS_BUILD_IO_HAS_STATS = (1u << 3),
    CAPS_BUILD_OBJECT_HASNAME = (1u << 4),
    CAPS_BUILD_DESC_DYNAMIC = (1u << 5),
  };

  explicit CIOCapabilities(CDescObject &desc)
    : CIOCommon(desc)
  {
  }

  ~CIOCapabilities() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "capabilities";
  }
#endif

  int configure() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;
  int getDataAtImpl(IODataCmn &data, size_t len, size_t offset) override;
  size_t getDataSize() const override;
  size_t getDataDim() const override;

  bool isRead() const override
  {
    return true;
  }

  bool isWrite() const override
  {
    return false;
  }

  bool isNotify() const override
  {
    return false;
  }

  bool isBatch() const override
  {
    return false;
  }

  bool isSeekable() const override
  {
    return true;
  }

  using ObjectIdHelper =
    CIOCommon::IOObjectIdHelperNoTS<IO_CLASS_CAPABILITIES, SObjectId::DTYPE_BLOCK>;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return ObjectIdHelper::create(inst);
  }

private:
  constexpr static size_t CAPS_HDR_SIZE = 8;
  constexpr static uint8_t CAPS_VERSION = 2;
  constexpr static size_t CAPS_BITMAP_WORDS = 16;
  constexpr static size_t CAPS_TOTAL_SIZE = 512;
  constexpr static size_t CAPS_BITMAP_BYTES = (CAPS_BITMAP_WORDS * 4);
  constexpr static size_t CAPS_PAYLOAD_SIZE = CAPS_TOTAL_SIZE - CAPS_HDR_SIZE;
  constexpr static size_t CAPS_IO_BITMAP_OFFSET = 0;
  constexpr static size_t CAPS_PROG_BITMAP_OFFSET = CAPS_BITMAP_BYTES;
  constexpr static size_t CAPS_PROTO_BITMAP_OFFSET = CAPS_BITMAP_BYTES * 2;
  constexpr static size_t CAPS_META_OFFSET = CAPS_BITMAP_BYTES * 3;
  constexpr static size_t CAPS_META_WORDS = (CAPS_PAYLOAD_SIZE - (CAPS_BITMAP_BYTES * 3)) / 4;

  uint8_t payloadBuf[CAPS_PAYLOAD_SIZE];

  int configureDesc(const CDescObject &desc);
  int configurePayload();
  void buildIoBitmap();
  void buildProgBitmap();
  void buildProtoBitmap();
  void buildMetaPayload();
  void setBitmapBit(uint8_t *bitmap, uint16_t bit);
  void setMetaWord(uint16_t index, uint32_t value);
  size_t copyFromBlob(uint8_t *dst, size_t offset, size_t len) const;
};

} // Namespace dawn
