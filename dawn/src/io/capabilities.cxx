// dawn/src/io/capabilities.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/capabilities.hxx"

#include <errno.h>

#include <cstring>

#include "dawn/prog/common.hxx"
#include "dawn/proto/common.hxx"

using namespace dawn;

int CIOCapabilities::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;
  size_t i = 0;

  for (i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      switch (item->cfgid.s.cls)
        {
          case CIOCommon::IO_CLASS_ANY:
            {
              offset += cfgCmnOffset(item);
              break;
            }

          default:
            {
              DAWNERR("unsupported capabilities cfg 0x08%" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

void CIOCapabilities::setBitmapBit(uint8_t *bitmap, uint16_t bit)
{
  uint16_t by;
  uint16_t bi;

  by = static_cast<uint16_t>(bit / 8u);
  bi = static_cast<uint16_t>(bit % 8u);

  if (by < CAPS_BITMAP_BYTES)
    {
      bitmap[by] |= static_cast<uint8_t>(1u << bi);
    }
}

void CIOCapabilities::buildIoBitmap()
{
  uint8_t *ioBitmap;

  ioBitmap = payloadBuf + CAPS_IO_BITMAP_OFFSET;
  std::memset(ioBitmap, 0, CAPS_BITMAP_BYTES);

#ifdef CONFIG_DAWN_IO_CONFIG
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_CONFIG);
#endif
#ifdef CONFIG_DAWN_IO_TRIGGER
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_TRIGGER);
#endif
#ifdef CONFIG_DAWN_IO_DESCRIPTOR
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_DESCRIPTOR);
#endif
#ifdef CONFIG_DAWN_IO_CONTROL
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_CONTROL);
#endif
#ifdef CONFIG_DAWN_IO_CAPABILITIES
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_CAPABILITIES);
#endif
#ifdef CONFIG_DAWN_IO_DESC_SELECTOR
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_DESC_SELECTOR);
#endif
#ifdef CONFIG_DAWN_IO_DUMMY
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_DUMMY);
#endif
#ifdef CONFIG_DAWN_IO_TIMESTAMPIO
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_TIMESTAMP);
#endif
#ifdef CONFIG_DAWN_IO_RANDIO
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_RAND);
#endif
#ifdef CONFIG_DAWN_IO_DUMMY_NOTIFY
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_DUMMY_NOTIFY);
#endif
#ifdef CONFIG_DAWN_IO_FILE
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_FILE);
#endif
#ifdef CONFIG_DAWN_IO_SENSOR
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_ACCELEROMETER);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_MAGNETICFIELD);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_GYROSCOPE);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_LIGHT);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_BAROMETER);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PROXIMITY);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_HUMIDITY);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_TEMPERATURE);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_ATEMPERATURE);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_RGB);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_IR);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_UV);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_GAS);
#endif
#ifdef CONFIG_DAWN_IO_SENSOR_PRODUCER
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_ACCELEROMETER);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_MAGNETICFIELD);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_GYROSCOPE);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_LIGHT);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_BAROMETER);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_PROXIMITY);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_HUMIDITY);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_TEMPERATURE);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_ATEMPERATURE);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_RGB);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_IR);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_UV);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SENSOR_PRODUCER_GAS);
#endif
#ifdef CONFIG_DAWN_IO_SYSINFO
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SYSTEM_UPTIME);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SYSTEM_CPULOAD);
#endif
#ifdef CONFIG_DAWN_IO_BOARDCTL
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SYSTEM_RESET);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SYSTEM_RESETCAUSE);
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SYSTEM_POWEROFF);
#endif
#ifdef CONFIG_DAWN_IO_UNAME
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SYSTEM_HOSTNAME);
#endif
#ifdef CONFIG_DAWN_IO_UUID
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SYSTEM_UUID);
#endif
#ifdef CONFIG_DAWN_IO_SYSTIME
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_SYSTEM_SYSTEMTIME);
#endif
#ifdef CONFIG_DAWN_IO_GPI
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_GPI_SINGLE);
#endif
#ifdef CONFIG_DAWN_IO_GPO
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_GPO_SINGLE);
#endif
#ifdef CONFIG_DAWN_IO_BUTTONS
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_BUTTONS);
#endif
#ifdef CONFIG_DAWN_IO_LEDS
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_LEDS);
#endif
#ifdef CONFIG_DAWN_IO_RGB_LED
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_RGBLED);
#endif
#ifdef CONFIG_DAWN_IO_PWM
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_PWM);
#endif
#ifdef CONFIG_DAWN_IO_ADC_FETCH
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_ADC_FETCH);
#endif
#ifdef CONFIG_DAWN_IO_ADC_SYNC
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_ADC_SYNC);
#endif
#ifdef CONFIG_DAWN_IO_ADC_STREAM
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_ADC_STREAM);
#endif
#ifdef CONFIG_DAWN_IO_DAC
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_DAC);
#endif
#ifdef CONFIG_DAWN_IO_ENCODER
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_ENCODER);
#endif
#ifdef CONFIG_DAWN_IO_ENCODER_INDEX
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_ENCODER_INDEX);
#endif
#ifdef CONFIG_DAWN_IO_VIRT
  setBitmapBit(ioBitmap, CIOCommon::IO_CLASS_VIRT);
#endif
}

void CIOCapabilities::buildProgBitmap()
{
  uint8_t *progBitmap;

  progBitmap = payloadBuf + CAPS_PROG_BITMAP_OFFSET;
  std::memset(progBitmap, 0, CAPS_BITMAP_BYTES);

#ifdef CONFIG_DAWN_PROG_STATS_MIN
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_STATS_MIN);
#endif
#ifdef CONFIG_DAWN_PROG_STATS_MAX
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_STATS_MAX);
#endif
#ifdef CONFIG_DAWN_PROG_STATS_AVG
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_STATS_AVG);
#endif
#ifdef CONFIG_DAWN_PROG_STATS_SUM
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_STATS_SUM);
#endif
#ifdef CONFIG_DAWN_PROG_STATS_COUNT
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_STATS_COUNT);
#endif
#ifdef CONFIG_DAWN_PROG_STATS_RMS
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_STATS_RMS);
#endif
#ifdef CONFIG_DAWN_PROG_SAMPLING
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_SAMPLING);
#endif
#ifdef CONFIG_DAWN_PROG_DUMMY
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_DUMMY);
#endif
#ifdef CONFIG_DAWN_PROG_ADJUST
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_ADJUST);
#endif
#ifdef CONFIG_DAWN_PROG_GATEWAY
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_GATEWAY);
#endif
#ifdef CONFIG_DAWN_PROG_LATEST
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_LATEST);
#endif
#ifdef CONFIG_DAWN_PROG_REDIRECT
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_REDIRECT);
#endif
#ifdef CONFIG_DAWN_PROG_MOVING_AVG
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_MOVING_AVG);
#endif
#ifdef CONFIG_DAWN_PROG_IIR_FILTER
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_IIR_FILTER);
#endif
#ifdef CONFIG_DAWN_PROG_THRESHOLD
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_THRESHOLD);
#endif
#ifdef CONFIG_DAWN_PROG_THRESHOLD_VALUE
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_THRESHOLD_VALUE);
#endif
#ifdef CONFIG_DAWN_PROG_BUFFER
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_BUFFER);
#endif
#ifdef CONFIG_DAWN_PROG_SEQUENCER
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_SEQUENCER);
#endif
#ifdef CONFIG_DAWN_PROG_BITSPLIT
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_BITSPLIT);
#endif
#ifdef CONFIG_DAWN_PROG_TOGGLE
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_TOGGLE);
#endif
#ifdef CONFIG_DAWN_PROG_COUNTER
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_COUNTER);
#endif
#ifdef CONFIG_DAWN_PROG_SWITCH
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_SWITCH);
#endif
#ifdef CONFIG_DAWN_PROG_EXPRESSION
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_EXPRESSION);
#endif
#ifdef CONFIG_DAWN_PROG_SELECTOR
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_SELECTOR);
#endif
#ifdef CONFIG_DAWN_PROG_BITPACK
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_BITPACK);
#endif
#ifdef CONFIG_DAWN_PROG_VECPACK
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_VECPACK);
#endif
#ifdef CONFIG_DAWN_PROG_VECSPLIT
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_VECSPLIT);
#endif
#ifdef CONFIG_DAWN_PROG_CONFIGWRITER
  setBitmapBit(progBitmap, CProgCommon::PROG_CLASS_CONFIGWRITER);
#endif
}

void CIOCapabilities::buildProtoBitmap()
{
  uint8_t *protoBitmap;

  protoBitmap = payloadBuf + CAPS_PROTO_BITMAP_OFFSET;
  std::memset(protoBitmap, 0, CAPS_BITMAP_BYTES);

#ifdef CONFIG_DAWN_PROTO_DUMMY
  setBitmapBit(protoBitmap, CProtoCommon::PROTO_CLASS_DUMMY);
#endif
#ifdef CONFIG_DAWN_PROTO_NIMBLE_PERIPHERAL
  setBitmapBit(protoBitmap, CProtoCommon::PROTO_CLASS_NIMBLE_PRPH);
#endif
#ifdef CONFIG_DAWN_PROTO_NXSCOPE_DUMMY
  setBitmapBit(protoBitmap, CProtoCommon::PROTO_CLASS_NXSCOPE_DUMMY);
#endif
#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SERIAL
  setBitmapBit(protoBitmap, CProtoCommon::PROTO_CLASS_NXSCOPE_SERIAL);
#endif
#ifdef CONFIG_DAWN_PROTO_NXSCOPE_UDP
  setBitmapBit(protoBitmap, CProtoCommon::PROTO_CLASS_NXSCOPE_UDP);
#endif
#ifdef CONFIG_DAWN_PROTO_SHELL_PRETTY
  setBitmapBit(protoBitmap, CProtoCommon::PROTO_CLASS_SHELL_STD);
#endif
#ifdef CONFIG_DAWN_PROTO_SERIAL
  setBitmapBit(protoBitmap, CProtoCommon::PROTO_CLASS_SERIAL);
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_RTU
  setBitmapBit(protoBitmap, CProtoCommon::PROTO_CLASS_MODBUS_RTU);
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_TCP
  setBitmapBit(protoBitmap, CProtoCommon::PROTO_CLASS_MODBUS_TCP);
#endif
#ifdef CONFIG_DAWN_PROTO_CAN
  setBitmapBit(protoBitmap, CProtoCommon::PROTO_CLASS_CAN);
#endif
#ifdef CONFIG_DAWN_PROTO_UDP
  setBitmapBit(protoBitmap, CProtoCommon::PROTO_CLASS_UDP);
#endif
#ifdef CONFIG_DAWN_PROTO_IPC
  setBitmapBit(protoBitmap, CProtoCommon::PROTO_CLASS_IPC);
#endif
}

void CIOCapabilities::setMetaWord(uint16_t index, uint32_t value)
{
  size_t off;

  if (index >= CAPS_META_WORDS)
    {
      return;
    }

  off = CAPS_META_OFFSET + static_cast<size_t>(index) * 4u;
  payloadBuf[off + 0] = static_cast<uint8_t>(value & 0xffu);
  payloadBuf[off + 1] = static_cast<uint8_t>((value >> 8) & 0xffu);
  payloadBuf[off + 2] = static_cast<uint8_t>((value >> 16) & 0xffu);
  payloadBuf[off + 3] = static_cast<uint8_t>((value >> 24) & 0xffu);
}

void CIOCapabilities::buildMetaPayload()
{
  uint32_t dtypeBitmap;
  uint32_t ioFlagsBitmap;
  uint32_t buildFlags;

  std::memset(payloadBuf + CAPS_META_OFFSET, 0, CAPS_PAYLOAD_SIZE - CAPS_META_OFFSET);

  dtypeBitmap = 0;
  ioFlagsBitmap = 0;
  buildFlags = 0;

  // DTYPE_ANY
  dtypeBitmap |= (1u << SObjectId::DTYPE_ANY);
#ifdef CONFIG_DAWN_DTYPE_BOOL
  dtypeBitmap |= (1u << SObjectId::DTYPE_BOOL);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT8
  dtypeBitmap |= (1u << SObjectId::DTYPE_INT8);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
  dtypeBitmap |= (1u << SObjectId::DTYPE_UINT8);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
  dtypeBitmap |= (1u << SObjectId::DTYPE_INT16);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
  dtypeBitmap |= (1u << SObjectId::DTYPE_UINT16);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
  dtypeBitmap |= (1u << SObjectId::DTYPE_INT32);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
  dtypeBitmap |= (1u << SObjectId::DTYPE_UINT32);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT64
  dtypeBitmap |= (1u << SObjectId::DTYPE_INT64);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT64
  dtypeBitmap |= (1u << SObjectId::DTYPE_UINT64);
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
  dtypeBitmap |= (1u << SObjectId::DTYPE_FLOAT);
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
  dtypeBitmap |= (1u << SObjectId::DTYPE_DOUBLE);
#endif
#ifdef CONFIG_DAWN_DTYPE_B16
  dtypeBitmap |= (1u << SObjectId::DTYPE_B16);
#endif
#ifdef CONFIG_DAWN_DTYPE_UB16
  dtypeBitmap |= (1u << SObjectId::DTYPE_UB16);
#endif
#ifdef CONFIG_DAWN_DTYPE_CHAR
  dtypeBitmap |= (1u << SObjectId::DTYPE_CHAR);
#endif
#ifdef CONFIG_DAWN_DTYPE_BLOCK
  dtypeBitmap |= (1u << SObjectId::DTYPE_BLOCK);
#endif

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  ioFlagsBitmap |= (1u << 0);
#endif

  buildFlags |= CAPS_BUILD_OS_NUTTX;
#ifdef CONFIG_FS_PROCFS
  buildFlags |= CAPS_BUILD_FILESYSTEM;
#endif
#ifdef CONFIG_DAWN_IO_NOTIFY
  buildFlags |= CAPS_BUILD_IO_NOTIFY;
#endif
#ifdef CONFIG_DAWN_IO_HAS_STATS
  buildFlags |= CAPS_BUILD_IO_HAS_STATS;
#endif
#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  buildFlags |= CAPS_BUILD_OBJECT_HASNAME;
#endif
#if CONFIG_DAWN_DESC_SLOTS > 1
  buildFlags |= CAPS_BUILD_DESC_DYNAMIC;
#endif

  // v2 metadata layout:
  // [0] dtype bits lo, [1] dtype bits hi,
  // [2] io flags lo, [3] io flags hi,
  // [4] build flags lo, [5] build flags hi,
  // [6] descriptor slots, [7] descriptor slot size,
  // [8] max io class id, [9] max prog class id, [10] max proto class id,
  // [11..] reserved.
  setMetaWord(0, dtypeBitmap);
  setMetaWord(1, 0);
  setMetaWord(2, ioFlagsBitmap);
  setMetaWord(3, 0);
  setMetaWord(4, buildFlags);
  setMetaWord(5, 0);
  setMetaWord(6, CONFIG_DAWN_DESC_SLOTS);
  setMetaWord(7, CONFIG_DAWN_DESC_SLOT_SIZE);
  setMetaWord(8, 0x1ffu);
  setMetaWord(9, 0x1ffu);
  setMetaWord(10, 0x1ffu);
  setMetaWord(11, 0);
}

int CIOCapabilities::configurePayload()
{
  std::memset(payloadBuf, 0, sizeof(payloadBuf));
  buildIoBitmap();
  buildProgBitmap();
  buildProtoBitmap();
  buildMetaPayload();
  return OK;
}

size_t CIOCapabilities::copyFromBlob(uint8_t *dst, size_t offset, size_t len) const
{
  uint8_t hdr[CAPS_HDR_SIZE];
  size_t copied;
  size_t hstart;
  size_t hcopy;
  size_t pstart;
  size_t pcopy;

  hdr[0] = CAPS_VERSION;
  hdr[1] = 0;
  hdr[2] = static_cast<uint8_t>(CAPS_PAYLOAD_SIZE & 0xff);
  hdr[3] = static_cast<uint8_t>((CAPS_PAYLOAD_SIZE >> 8) & 0xff);
  hdr[4] = 0;
  hdr[5] = 0;
  hdr[6] = 0;
  hdr[7] = 0;

  copied = 0;

  if (offset < CAPS_HDR_SIZE)
    {
      hstart = offset;
      hcopy = CAPS_HDR_SIZE - hstart;
      if (hcopy > len)
        {
          hcopy = len;
        }

      std::memcpy(dst, &hdr[hstart], hcopy);
      copied += hcopy;
    }

  if (copied < len)
    {
      if (offset + copied < CAPS_HDR_SIZE)
        {
          pstart = 0;
        }
      else
        {
          pstart = offset + copied - CAPS_HDR_SIZE;
        }

      pcopy = CAPS_PAYLOAD_SIZE - pstart;
      if (pcopy > (len - copied))
        {
          pcopy = len - copied;
        }

      if (pcopy > 0)
        {
          std::memcpy(dst + copied, payloadBuf + pstart, pcopy);
          copied += pcopy;
        }
    }

  return copied;
}

CIOCapabilities::~CIOCapabilities()
{
  deinit();
}

int CIOCapabilities::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      return ret;
    }

  return configurePayload();
}

int CIOCapabilities::deinit()
{
  return OK;
}

int CIOCapabilities::getDataImpl(IODataCmn &data, size_t len)
{
  (void)data;
  (void)len;
  return -ENOTSUP;
}

int CIOCapabilities::setDataImpl(IODataCmn &data)
{
  (void)data;
  return -ENOTSUP;
}

int CIOCapabilities::getDataAtImpl(IODataCmn &data, size_t len, size_t offset)
{
  size_t total;
  size_t avail;
  size_t toCopy;

  if (len != 1)
    {
      return -EINVAL;
    }

  total = getDataSize();
  if (offset >= total)
    {
      return -EINVAL;
    }

  avail = total - offset;
  toCopy = data.getDataSize() < avail ? data.getDataSize() : avail;
  copyFromBlob(static_cast<uint8_t *>(data.getDataPtr()), offset, toCopy);

  return OK;
}

size_t CIOCapabilities::getDataSize() const
{
  return CAPS_TOTAL_SIZE;
}

size_t CIOCapabilities::getDataDim() const
{
  return getDataSize();
}
