// dawn/include/dawn/proto/modbus/regs.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <mutex>
#include <vector>

#include "dawn/common/object.hxx"
#include "dawn/porting/config.hxx"
#include <nxmodbus/nxmodbus.h>

namespace dawn
{
// Forward declaration

struct io_ddata_t;
class CIOCommon;

/**
 * @brief Modbus register management base class (shared RTU/TCP logic).
 *
 * CProtoModbusRegs provides common register-to-I/O mapping logic used by both
 * Modbus RTU and TCP protocol implementations.
 */

class CProtoModbusRegs
{
public:
  enum
  {
    MODBUS_TYPE_COIL = 1,            ///< Read-write bits (1 byte/bit).
    MODBUS_TYPE_DISCRETE = 2,        ///< Read-only bits (1 byte/bit).
    MODBUS_TYPE_COIL_PACKED = 3,     ///< Read-write bits packed in bytes.
    MODBUS_TYPE_DISCRETE_PACKED = 4, ///< Read-only bits packed in bytes.
    MODBUS_TYPE_INPUT = 5,           ///< Read-only 16-bit words.
    MODBUS_TYPE_HOLDING = 6,         ///< Read-write 16-bit words.
    MODBUS_TYPE_SEEKABLE = 7,        ///< Seekable window over holding regs.
  };

  /** @brief I/O binding configuration loaded from descriptor. */

  struct
  {
    uint32_t type;    ///< Register type (MODBUS_TYPE_*).
    uint32_t config;  ///< Register-type private configuration word.
    uint32_t start;   ///< Start register address (0x0000-0xFFFF).
    uint32_t size;    ///< Number of I/O objects in this group.
    uint32_t objid[]; ///< Array of I/O object IDs.
  } typedef SProtoModbusIOBind;

  /** @brief Runtime register group state. */

  struct
  {
    const SProtoModbusIOBind *cfg;    ///< Reference to bind config.
    std::vector<io_ddata_t *> iodata; ///< I/O data for each object.
    std::mutex mutex;                 ///< Thread-safe access guard.
    void *data;                       ///< Register data buffer.
    size_t regs;                      ///< Number of registers.
    size_t size;                      ///< Data buffer size in bytes.
    int ios;                          ///< Number of I/O objects.
  } typedef SProtoModbusRegs;

  CProtoModbusRegs()
    : initialized(false)
  {
  }

  ~CProtoModbusRegs();

  void valloc_push_back(SProtoModbusIOBind *alloc);
  int createRegs();
  int destroyRegs();

#ifdef CONFIG_DAWN_PROTO_MODBUS_COIL
  int coilsCb(uint8_t *buf, uint16_t addr, uint16_t ncoils, enum nxmb_regmode_e mode, void *priv);
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_DISCRETE
  int discreteCb(uint8_t *buf, uint16_t addr, uint16_t ndiscrete, void *priv);
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_INPUT
  int inputCb(uint8_t *buf, uint16_t addr, uint16_t nregs, void *priv);
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_HOLDING
  int holdingCb(uint8_t *buf, uint16_t addr, uint16_t nregs, enum nxmb_regmode_e mode, void *priv);
#endif

private:
#ifdef CONFIG_DAWN_PROTO_MODBUS_SEEKABLE
  struct
  {
    SProtoModbusRegs *reg;                 ///< Owning register group.
    size_t windowRegs;                     ///< Seekable data window width.
    std::vector<size_t> seekOffsets;       ///< Current seek offsets per I/O.
    std::vector<size_t> readProgressWords; ///< Read progress per I/O window.
  } typedef SSeekableState;
#endif

  std::vector<SProtoModbusIOBind *> valloc;  ///< I/O bindings (from descriptor).
#ifdef CONFIG_DAWN_PROTO_MODBUS_COIL
  std::vector<SProtoModbusRegs *> vcoils;    ///< Coil register groups.
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_DISCRETE
  std::vector<SProtoModbusRegs *> vdiscrete; ///< Discrete input register groups.
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_INPUT
  std::vector<SProtoModbusRegs *> vinput;    ///< Input register groups.
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_HOLDING
  std::vector<SProtoModbusRegs *> vholding;  ///< Holding register groups.
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_SEEKABLE
  std::vector<SSeekableState *> vseekable;   ///< Runtime states for MODBUS_TYPE_SEEKABLE groups.
#endif
  bool initialized;                          ///< Initialization completion flag.

  virtual CIOCommon *getIO_(SObjectId::ObjectId id) = 0;

  SProtoModbusRegs *findGroup(uint16_t addr,
                              uint16_t n,
                              const std::vector<SProtoModbusRegs *> &regs);

  const SProtoModbusRegs *hasOverlap(const std::vector<SProtoModbusRegs *> &regs,
                                     uint32_t start,
                                     size_t count) const;

  void cleanupGroups(std::vector<SProtoModbusRegs *> &groups);

#if defined(CONFIG_DAWN_PROTO_MODBUS_COIL) || defined(CONFIG_DAWN_PROTO_MODBUS_DISCRETE)
  int regReadWriteCoil(uint8_t *buff, uint16_t n, int index, bool read, SProtoModbusRegs *reg);

  int regReadWriteCoilPacked(uint8_t *buff,
                             uint16_t n,
                             int index,
                             bool read,
                             SProtoModbusRegs *reg);
#endif

#if defined(CONFIG_DAWN_PROTO_MODBUS_INPUT) || defined(CONFIG_DAWN_PROTO_MODBUS_HOLDING)
  int regReadWriteHolding(uint8_t *buff, uint16_t n, int index, bool read, SProtoModbusRegs *reg);

  int standardRegRW(uint8_t *buff, uint16_t n, int index, bool read, SProtoModbusRegs *reg);

#  ifdef CONFIG_DAWN_PROTO_MODBUS_SEEKABLE
  int seekableRegRW(uint8_t *buff,
                    uint16_t n,
                    int index,
                    bool read,
                    SProtoModbusRegs *reg,
                    SSeekableState *seekState);
#  endif
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_SEEKABLE
  SSeekableState *findSeekableState(SProtoModbusRegs *reg);
#endif

  int finalizeRegGroup(SProtoModbusRegs *tmp,
                       const SProtoModbusIOBind *v,
                       size_t seekWindowRegs,
                       bool addSeekableState);
};

} // Namespace dawn
