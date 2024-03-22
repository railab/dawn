// dawn/src/prog/adjust.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/adjust.hxx"

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

template<typename SRC, typename DST, typename PARAM>
static void applyAdjust(const void *src,
                        void *dst,
                        size_t n,
                        uint32_t rawScale,
                        uint32_t rawOffset,
                        PARAM (*toCfg)(uint32_t))
{
  PARAM scale = toCfg(rawScale);
  PARAM offset = toCfg(rawOffset);
  const SRC *in = static_cast<const SRC *>(src);
  DST *out = static_cast<DST *>(dst);

  for (size_t i = 0; i < n; i++)
    {
      out[i] = static_cast<DST>(in[i]) * scale + offset;
    }
}

template<typename SRC>
static void applyAdjustB16(const void *src,
                           void *dst,
                           size_t n,
                           uint32_t rawScale,
                           uint32_t rawOffset)
{
  b16_t scale = SObjectCfg::cfgToB16(rawScale);
  b16_t offset = SObjectCfg::cfgToB16(rawOffset);
  const SRC *in = static_cast<const SRC *>(src);
  b16_t *out = static_cast<b16_t *>(dst);

  for (size_t i = 0; i < n; i++)
    {
      b16_t tmp1 = itob16(in[i]);
      b16_t tmp2 = b16mulb16(tmp1, scale);
      out[i] = tmp2 + offset;
    }
}

static void applyAdjustFloatToB16(const void *src,
                                  void *dst,
                                  size_t n,
                                  uint32_t rawScale,
                                  uint32_t rawOffset)
{
  b16_t scale = SObjectCfg::cfgToB16(rawScale);
  b16_t offset = SObjectCfg::cfgToB16(rawOffset);
  const float *in = static_cast<const float *>(src);
  b16_t *out = static_cast<b16_t *>(dst);

  for (size_t i = 0; i < n; i++)
    {
      b16_t tmp1 = ftob16(in[i]);
      b16_t tmp2 = b16mulb16(tmp1, scale);
      out[i] = tmp2 + offset;
    }
}

static double cfgToDouble(uint32_t raw)
{
  return static_cast<double>(SObjectCfg::cfgToF(raw));
}

template<typename SRC>
static void applyAdjustRealToB16(const void *src,
                                 void *dst,
                                 size_t n,
                                 uint32_t rawScale,
                                 uint32_t rawOffset)
{
  b16_t scale = SObjectCfg::cfgToB16(rawScale);
  b16_t offset = SObjectCfg::cfgToB16(rawOffset);
  const SRC *in = static_cast<const SRC *>(src);
  b16_t *out = static_cast<b16_t *>(dst);

  for (size_t i = 0; i < n; i++)
    {
      b16_t tmp1 = ftob16(static_cast<float>(in[i]));
      b16_t tmp2 = b16mulb16(tmp1, scale);
      out[i] = tmp2 + offset;
    }
}

template<typename SRC>
static bool applyAdjustToDst(int dstType,
                             const void *src,
                             void *dst,
                             size_t n,
                             uint32_t rawScale,
                             uint32_t rawOffset)
{
  switch (dstType)
    {
#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        applyAdjust<SRC, int8_t, int32_t>(src, dst, n, rawScale, rawOffset, SObjectCfg::cfgtoi32);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
        applyAdjust<SRC, uint8_t, uint32_t>(src, dst, n, rawScale, rawOffset, SObjectCfg::cfgToU32);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        applyAdjust<SRC, int16_t, int32_t>(src, dst, n, rawScale, rawOffset, SObjectCfg::cfgtoi32);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        applyAdjust<SRC, uint16_t, uint32_t>(
          src, dst, n, rawScale, rawOffset, SObjectCfg::cfgToU32);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        applyAdjust<SRC, int32_t, int32_t>(src, dst, n, rawScale, rawOffset, SObjectCfg::cfgtoi32);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        applyAdjust<SRC, uint32_t, uint32_t>(
          src, dst, n, rawScale, rawOffset, SObjectCfg::cfgToU32);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT64
      case SObjectId::DTYPE_INT64:
        applyAdjust<SRC, int64_t, int32_t>(src, dst, n, rawScale, rawOffset, SObjectCfg::cfgtoi32);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT64
      case SObjectId::DTYPE_UINT64:
        applyAdjust<SRC, uint64_t, uint32_t>(
          src, dst, n, rawScale, rawOffset, SObjectCfg::cfgToU32);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
        applyAdjust<SRC, float, float>(src, dst, n, rawScale, rawOffset, SObjectCfg::cfgToF);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
      case SObjectId::DTYPE_DOUBLE:
        applyAdjust<SRC, double, double>(src, dst, n, rawScale, rawOffset, cfgToDouble);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_B16
      case SObjectId::DTYPE_B16:
        applyAdjustB16<SRC>(src, dst, n, rawScale, rawOffset);
        return true;
#endif
      default:
        return false;
    }
}

template<>
bool applyAdjustToDst<float>(int dstType,
                             const void *src,
                             void *dst,
                             size_t n,
                             uint32_t rawScale,
                             uint32_t rawOffset)
{
  if (dstType == SObjectId::DTYPE_B16)
    {
#ifdef CONFIG_DAWN_DTYPE_B16
      applyAdjustFloatToB16(src, dst, n, rawScale, rawOffset);
      return true;
#else
      return false;
#endif
    }

  return applyAdjustToDst<const float>(dstType, src, dst, n, rawScale, rawOffset);
}

template<>
bool applyAdjustToDst<double>(int dstType,
                              const void *src,
                              void *dst,
                              size_t n,
                              uint32_t rawScale,
                              uint32_t rawOffset)
{
  if (dstType == SObjectId::DTYPE_B16)
    {
#ifdef CONFIG_DAWN_DTYPE_B16
      applyAdjustRealToB16<double>(src, dst, n, rawScale, rawOffset);
      return true;
#else
      return false;
#endif
    }

  return applyAdjustToDst<const double>(dstType, src, dst, n, rawScale, rawOffset);
}

static bool applyAdjustByType(int srcType,
                              int dstType,
                              const void *src,
                              void *dst,
                              size_t n,
                              uint32_t rawScale,
                              uint32_t rawOffset)
{
  switch (srcType)
    {
#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        return applyAdjustToDst<int8_t>(dstType, src, dst, n, rawScale, rawOffset);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
        return applyAdjustToDst<uint8_t>(dstType, src, dst, n, rawScale, rawOffset);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        return applyAdjustToDst<int16_t>(dstType, src, dst, n, rawScale, rawOffset);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        return applyAdjustToDst<uint16_t>(dstType, src, dst, n, rawScale, rawOffset);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        return applyAdjustToDst<int32_t>(dstType, src, dst, n, rawScale, rawOffset);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        return applyAdjustToDst<uint32_t>(dstType, src, dst, n, rawScale, rawOffset);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT64
      case SObjectId::DTYPE_INT64:
        return applyAdjustToDst<int64_t>(dstType, src, dst, n, rawScale, rawOffset);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT64
      case SObjectId::DTYPE_UINT64:
        return applyAdjustToDst<uint64_t>(dstType, src, dst, n, rawScale, rawOffset);
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
        return applyAdjustToDst<float>(dstType, src, dst, n, rawScale, rawOffset);
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
      case SObjectId::DTYPE_DOUBLE:
        return applyAdjustToDst<double>(dstType, src, dst, n, rawScale, rawOffset);
#endif
      default:
        return false;
    }
}

int CProgAdjust::ioNotifierCb(void *priv, io_ddata_t *data)
{
  CProgAdjust *obj = static_cast<CProgAdjust *>(priv);

  // Handle data

  obj->handle(data);
  return OK;
}

int CProgAdjust::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_ADJUST)
        {
          DAWNERR("Unsupported adjust config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_ADJUST_CFG_IOBIND:
            {
              offset += 1;

              if (item->cfgid.s.size != sizeof(SProgAdjustIOBind) / 4)
                {
                  DAWNERR("Invalid IOBIND config size %d, expected %zu\n",
                          item->cfgid.s.size,
                          sizeof(SProgAdjustIOBind) / 4);
                  return -EINVAL;
                }

              SProgAdjustIOBind *iobind =
                reinterpret_cast<SProgAdjustIOBind *>(desc.objectCfgItemAtOffset(offset));

              allocObject(iobind);
              offset += sizeof(SProgAdjustIOBind) / 4;
              break;
            }

          case PROG_ADJUST_CFG_PARAMS:
            {
              offset += 1;

              if (item->cfgid.s.size != sizeof(SProgAdjustParams) / 4)
                {
                  DAWNERR("Invalid PARAMS config size %d, expected %zu\n",
                          item->cfgid.s.size,
                          sizeof(SProgAdjustParams) / 4);
                  return -EINVAL;
                }

              const SProgAdjustParams *params =
                reinterpret_cast<SProgAdjustParams *>(desc.objectCfgItemAtOffset(offset));

              ioscale = params->scale;
              iooffset = params->offset;
              offset += sizeof(SProgAdjustParams) / 4;
              break;
            }

          default:
            {
              DAWNERR("Unsupported adjust config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

void CProgAdjust::handle(io_ddata_t *data)
{
  if (data == nullptr)
    {
      DAWNERR("Null data pointer in handle()\n");
      return;
    }

  void *data_ptr = data->getDataPtr();
  void *dst = outputData->getDataPtr();
  size_t n = outputData->getItems();

  if (!applyAdjustByType(srcType, outputType, data_ptr, dst, n, ioscale, iooffset))
    {
      DAWNERR("Invalid conversion from type %d to type %d\n", srcType, outputType);
      return;
    }

  output->setData(*outputData);
}

bool CProgAdjust::isSupportedConvType(int sourceType, int targetType)
{
  return applyAdjustByType(sourceType, targetType, nullptr, nullptr, 0, 0, 0);
}

int CProgAdjust::allocObject(SProgAdjustIOBind *alloc)
{
  DAWNINFO("allocate prog 0x%" PRIx32 " -> 0x%" PRIx32 "\n", alloc->objid.v, alloc->output.v);

  // Allocate source and output in map

  setObjectMapItem(alloc->objid.v, nullptr);
  setObjectMapItem(alloc->output.v, nullptr);

  // Store pointer to config for later

  cfg = alloc;

  return OK;
};

int CProgAdjust::bindPrepare()
{
  int ret;
  size_t dim;

  if (!src->isRead())
    {
      DAWNERR("Source IO 0x%" PRIx32 " is not readable\n", src->getIdV());
      return -EINVAL;
    }

#ifdef CONFIG_DAWN_IO_NOTIFY
  if (src->isNotify())
    {
      // Register callback if notify is supported

      ret = src->setNotifier(ioNotifierCb, 0, this);
      if (ret < 0)
        {
          DAWNERR("Set notifier failed for objId = 0x%" PRIx32 ": %d\n", src->getIdV(), ret);
          return ret;
        }
    }
#endif

  // Get dimension

  dim = src->getDataDim();

  // Initialize deferred virtual outputs and validate configured targets.

  ret = prepareWritableTarget(output, dim, src->isNotify());
  if (ret != OK)
    {
      DAWNERR("Output IO initialize failed: %d\n", ret);
      return ret;
    }

  if (output->getDataDim() != dim)
    {
      DAWNERR("Adjust output 0x%" PRIx32 " shape mismatch\n", output->getIdV());
      return -EINVAL;
    }

  return OK;
}

CProgAdjust::~CProgAdjust()
{
  deinit();
}

int CProgAdjust::configure()
{
  int ret;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("Adjust configure failed (error %d)\n", ret);
      return ret;
    }

  // Type of conversion

  srcType = SObjectId::objectIdGetDtype(cfg->objid.v);
  outputType = SObjectId::objectIdGetDtype(cfg->output.v);

  if (!isSupportedConvType(srcType, outputType))
    {
      DAWNERR("Unsupported conversion from type %d to type %d\n", srcType, outputType);
      return -EINVAL;
    }

  // By default, scale is set to 1 and offset set to 0 to allow type conversion

  if (ioscale == 0 && iooffset == 0)
    {
      switch (outputType)
        {
#if defined(CONFIG_DAWN_DTYPE_INT8) || defined(CONFIG_DAWN_DTYPE_INT16) || \
  defined(CONFIG_DAWN_DTYPE_INT32) || defined(CONFIG_DAWN_DTYPE_INT64)
          case SObjectId::DTYPE_INT8:
          case SObjectId::DTYPE_INT16:
          case SObjectId::DTYPE_INT32:
          case SObjectId::DTYPE_INT64:
            ioscale = SObjectCfg::i32ToCfg(1);
            iooffset = SObjectCfg::i32ToCfg(0);
            break;
#endif

#if defined(CONFIG_DAWN_DTYPE_UINT8) || defined(CONFIG_DAWN_DTYPE_UINT16) || \
  defined(CONFIG_DAWN_DTYPE_UINT32) || defined(CONFIG_DAWN_DTYPE_UINT64)
          case SObjectId::DTYPE_UINT8:
          case SObjectId::DTYPE_UINT16:
          case SObjectId::DTYPE_UINT32:
          case SObjectId::DTYPE_UINT64:
            ioscale = SObjectCfg::u32ToCfg(1);
            iooffset = SObjectCfg::u32ToCfg(0);
            break;
#endif

#ifdef CONFIG_DAWN_DTYPE_FLOAT
          case SObjectId::DTYPE_FLOAT:
            ioscale = SObjectCfg::fToCfg(1.0f);
            iooffset = SObjectCfg::fToCfg(0.0f);
            break;
#endif

#ifdef CONFIG_DAWN_DTYPE_DOUBLE
          case SObjectId::DTYPE_DOUBLE:
            ioscale = SObjectCfg::fToCfg(1.0f);
            iooffset = SObjectCfg::fToCfg(0.0f);
            break;
#endif

#ifdef CONFIG_DAWN_DTYPE_B16
          case SObjectId::DTYPE_B16:
            ioscale = SObjectCfg::fToB16ToCfg(1.0f);
            iooffset = SObjectCfg::fToB16ToCfg(0.0f);
            break;
#endif

          default:
            DAWNERR("Unsupported output IO type %d for default scale/offset\n", outputType);
            return -EINVAL;
        }
    }

  // One-time initialization that depends on bindings is done in init()

  return OK;
}

int CProgAdjust::deinit()
{
  // Delete objects

  delete ioData;
  ioData = nullptr;
  delete outputData;
  outputData = nullptr;
  src = nullptr;
  output = nullptr;

  return OK;
}

int CProgAdjust::init()
{
  int ret;

  // Get IOs

  src = getIO(cfg->objid.v);
  if (!src)
    {
      return -EIO;
    }

  output = getIO(cfg->output.v);
  if (!output)
    {
      return -EIO;
    }

  // Complete binding

  ret = bindPrepare();
  if (ret != OK)
    {
      return ret;
    }

  // Create storage for IO data

  ioData = src->ddata_alloc(1);
  if (!ioData)
    {
      return -ENOMEM;
    }

  outputData = output->ddata_alloc(1);
  if (!outputData)
    {
      delete ioData;
      ioData = nullptr;
      return -ENOMEM;
    }

  return OK;
}

int CProgAdjust::refresh()
{
  int ret;

  ret = src->getData(*ioData, 1);
  if (ret != OK)
    {
      DAWNERR("Failed to get data from source IO (error %d)\n", ret);
      return ret;
    }

  handle(ioData);
  return OK;
}

int CProgAdjust::doStart()
{
  if (src->isNotify())
    {
      return OK;
    }

  return refresh();
};
