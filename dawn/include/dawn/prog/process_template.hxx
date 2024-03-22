// dawn/include/dawn/prog/process_template.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/ddata.hxx"
#include "dawn/io/common.hxx"
#include "dawn/prog/process.hxx"

namespace dawn
{
/** @brief Policy-based template for sample processing implementations.
 *
 * Generic template that consolidates common sample-processing logic with a
 * pluggable per-element policy.
 */

template<typename StatsPolicy>
class CProgProcessTemplate : public CProgProcess
{
private:
  /**
   * @brief Handle data sample with type dispatch.
   *
   * Dispatches to strongly-typed apply operation.
   *
   * @param[in] output Result output I/O object.
   * @param[in] data New sample data from source I/O.
   */

  template<typename T>
  void handleTemplate(CIOCommon *output,
                      io_ddata_t *data,
                      io_ddata_t *ioData,
                      io_ddata_t *outputData,
                      bool &initsample)
  {
    bool update = false;

    // Copy incoming sample to internal buffer.

    std::memcpy(ioData->getDataPtr(), data->getDataPtr(), ioData->getDataSize());

    // Initial sample processing.

    if (initsample)
      {
        output->setData(*ioData);
        std::memset(outputData->getDataPtr(), 0, outputData->getDataSize());
        initsample = false;
      }

    // Apply operation to all elements.

    for (size_t i = 0; i < ioData->getItems(); i++)
      {
        StatsPolicy::template apply<T>(i, ioData, outputData, update);
      }

    // Write updated result back to output I/O.

    if (update)
      {
        output->setData(*outputData);
      }
  }

protected:
  /**
   * @brief Process incoming sample.
   *
   * @param[in] output Result output I/O object.
   * @param[in] data Sample data from source I/O.
   * @param[in] ioData Per-binding input data buffer.
   * @param[in] outputData Per-binding output data buffer.
   * @param[in,out] initsample Per-binding initial sample flag.
   */

  void handle(CIOCommon *output,
              io_ddata_t *data,
              io_ddata_t *ioData,
              io_ddata_t *outputData,
              bool &initsample) override
  {
    switch (data->getDtype())
      {
#ifdef CONFIG_DAWN_DTYPE_INT8
        case SObjectId::DTYPE_INT8:
          {
            this->handleTemplate<int8_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT8
        case SObjectId::DTYPE_UINT8:
          {
            this->handleTemplate<uint8_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT16
        case SObjectId::DTYPE_INT16:
          {
            this->handleTemplate<int16_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT16
        case SObjectId::DTYPE_UINT16:
          {
            this->handleTemplate<uint16_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT32
        case SObjectId::DTYPE_INT32:
          {
            this->handleTemplate<int32_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT32
        case SObjectId::DTYPE_UINT32:
          {
            this->handleTemplate<uint32_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT64
        case SObjectId::DTYPE_INT64:
          {
            this->handleTemplate<int64_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT64
        case SObjectId::DTYPE_UINT64:
          {
            this->handleTemplate<uint64_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_FLOAT
        case SObjectId::DTYPE_FLOAT:
          {
            this->handleTemplate<float>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_DOUBLE
        case SObjectId::DTYPE_DOUBLE:
          {
            this->handleTemplate<double>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

        default:
          {
            DAWNERR("process: unsupported dtype %d\n", data->getDtype());
          }
      }
  }

public:
  /**
   * @brief Construct the template-based processing Program.
   *
   * @param[in] desc Descriptor with process configuration.
   */

  explicit CProgProcessTemplate(CDescObject &desc)
    : CProgProcess(desc)
  {
  }
};

} // namespace dawn
