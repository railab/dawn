// dawn/include/dawn/prog/process.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vector>

#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
class CIOCommon;
class IIOCommon;
class io_ddata_t;

/**
 * @brief Base class for callback-driven sample processing Program objects.
 *
 * CProgProcess provides shared infrastructure for Programs that consume
 * samples from a source IO and publish processed results through an output IO.
 * Derived classes implement the processing policy in handle().
 */

class CProgProcess : public CProgCommon
{
public:
  /** @brief Configuration IDs. */

  enum
  {
    PROG_STATS_CFG_FIRST = 0,  ///< First config ID
    PROG_STATS_CFG_IOBIND = 1, ///< I/O binding configuration
    PROG_STATS_CFG_LAST = 31   ///< Last config ID
  };

  /** @brief Configuration structure for process I/O binding. */

  struct
  {
    SObjectId::UObjectId objid;  ///< Source I/O object ID
    SObjectId::UObjectId output; ///< Output I/O object ID
  } typedef SProgStatsIOBind;

  /**
   * @brief Construct a a Program from descriptor.
   *
   * @param[in] desc Descriptor containing process configuration.
   */

  CProgProcess(CDescObject &desc)
    : CProgCommon(desc)
  {
  }

  /**
   * @brief Destruct the a Program.
   *
   * Cleans up output bindings and callbacks.
   */

  ~CProgProcess() override;

  /**
   * @brief Configure Program from descriptor.
   *
   * @return OK on success, or error code on failure.
   */

  int configure() override;

  /**
   * @brief One-time initialize Program after bindings are resolved.
   *
   * @return OK on success, or error code on failure.
   */

  int init() override;

  /**
   * @brief Deinitialize Program.
   *
   * @return OK on success, or error code on failure.
   */

  int deinit() override;

  /**
   * @brief Start the Program.
   *
   * @return OK on success, or error code on failure.
   */

  int doStart() override;

  /**
   * @brief Stop the Program.
   *
   * @return OK on success, or error code on failure.
   */

  int doStop() override;

  /**
   * @brief Check if a background thread is active.
   *
   * @return Always false.
   */

  bool hasThread() const override;

  /**
   * @brief Execute a trigger command.
   *
   * @param[in] cmd Command identifier (EObjectCmd value).
   * @return OK on success, -ENOTSUP if not supported.
   */

  int trigger(uint8_t cmd) override;

protected:
  struct SBindState
  {
    virtual ~SBindState() = default;

    virtual void reset()
    {
    }
  };

private:
  /** @brief Runtime state for one source->virt binding. */

  struct SProcessBind
  {
    CProgProcess *owner = nullptr;
    SProgStatsIOBind cfg = {};
    CIOCommon *src = nullptr;
    CIOCommon *output = nullptr;
    io_ddata_t *ioData = nullptr;
    io_ddata_t *outputData = nullptr;
    SBindState *state = nullptr;
    bool initsample = false;
    bool active = false;
  };

  /** @brief All configured source->virt bindings. */

  std::vector<SProcessBind> binds;

  /**
   * @brief I/O notification callback.
   *
   * @param[in] priv Callback context (CProgProcess*).
   * @param[in] data New data from source I/O.
   * @return OK (notification consumed).
   */

  static int ioNotifierCb(void *priv, io_ddata_t *data);

  /**
   * @brief Configure object from descriptor.
   *
   * @param[in] desc Descriptor containing configuration.
   * @return OK on success, negative error code on failure.
   */

  int configureDesc(const CDescObject &desc);

  /**
   * @brief Allocate object configuration.
   *
   * @param[in] alloc Pointer to configuration to populate.
   * @return OK on success, or error code on failure.
   */

  int allocObject(const SProgStatsIOBind *alloc);

  /**
   * @brief Prepare one source-to-output binding.
   *
   * @return OK on success, or error code on failure.
   */

  int bindPrepare(SProcessBind *bind);

protected:
  /**
   * @brief Parse program-specific config item (optional extension hook).
   *
   * @return OK when item was handled, negative error otherwise.
   */

  virtual int configureExtraCfgItem(const CDescObject &desc,
                                    const SObjectCfg::SObjectCfgItem *item,
                                    size_t &offset);

  /**
   * @brief Process incoming sample.
   *
   * @param[in] output Result output I/O object.
   * @param[in] data New sample data from source I/O.
   * @param[in] ioData Per-binding input data buffer.
   * @param[in] outputData Per-binding output data buffer.
   * @param[in,out] initsample Per-binding initial sample flag.
   */

  virtual void handle(CIOCommon *output,
                      io_ddata_t *data,
                      io_ddata_t *ioData,
                      io_ddata_t *outputData,
                      bool &initsample) = 0;

  /**
   * @brief Allocate optional per-binding derived state.
   *
   * Called once during init() after source/output buffers are allocated.
   * Derived classes that do not need extra state can use the default null
   * state.
   */

  virtual int bindStateAlloc(CIOCommon *src,
                             CIOCommon *output,
                             io_ddata_t *ioData,
                             io_ddata_t *outputData,
                             SBindState **state);

  /**
   * @brief Process incoming sample with optional per-binding state.
   *
   * The default implementation preserves the original handle() contract.
   */

  virtual void handleWithState(CIOCommon *output,
                               io_ddata_t *data,
                               io_ddata_t *ioData,
                               io_ddata_t *outputData,
                               bool &initsample,
                               void *state);

  /**
   * @brief Get the output IO for the first bind.
   *
   * Convenience for sub-classes that need to emit an initial value
   * during doStart() without iterating the internal bind vector.
   *
   * @return Pointer to the first bind's output IO, or nullptr.
   */
  CIOCommon *getOutputIO() const
  {
    if (!binds.empty())
      {
        return binds[0].output;
      }
    return nullptr;
  }

  /**
   * @brief Get the source IO for the first bind.
   *
   * Convenience for sub-classes that need to read the current source
   * value during doStart().
   *
   * @return Pointer to the first bind's source IO, or nullptr.
   */
  CIOCommon *getSourceIO() const
  {
    if (!binds.empty())
      {
        return binds[0].src;
      }
    return nullptr;
  }

  /**
   * @brief Common callback wrapper.
   *
   * @param[in] bind Per-binding state.
   * @param[in] data Sample data.
   */

  void handleCmn(SProcessBind *bind, io_ddata_t *data)
  {
    DAWNASSERT(bind != nullptr, "nullptr pointer");
    DAWNASSERT(data != nullptr, "nullptr pointer");

    // Handle by derived processing implementation

    this->handleWithState(
      bind->output, data, bind->ioData, bind->outputData, bind->initsample, bind->state);
  };
};
} // Namespace dawn
