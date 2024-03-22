// dawn/include/dawn/proto/simplebase.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <set>
#include <vector>

#include "dawn/porting/config.hxx"
#include "dawn/proto/common.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;
struct io_ddata_t;

/**
 * @brief Shared base for simple framed protocols.
 *
 * Provides the common request handlers, frame parsing, shared IO buffers, and
 * notification management used by the serial and UDP simple protocols.
 */

class CProtoSimpleBase : public CProtoCommon
{
public:
  /** @brief Frame structure constants. */

  static constexpr uint8_t FRAME_SYNC = 0xAA;
  static constexpr uint8_t FRAME_MIN_LEN = 6;
  static constexpr size_t FRAME_MAX_PAYLOAD = 1024;

  /** @brief Seek response constants. */

  static constexpr size_t SEEK_HDR_SIZE = 8;
  static constexpr size_t SEEK_CHUNK_CAP = FRAME_MAX_PAYLOAD - SEEK_HDR_SIZE;

  /** @brief Command IDs for frame exchange */

  enum
  {
    CMD_PING = 0x00,
    CMD_PONG = 0x01,
    CMD_GET_IO = 0x10,
    CMD_SET_IO = 0x11,
    CMD_GET_IO_SEEK = 0x14,
    CMD_SET_IO_SEEK = 0x15,
    CMD_GET_CFG = 0x12,
    CMD_SET_CFG = 0x13,
    CMD_GET_INFO = 0x20,
    CMD_LIST_IOS = 0x21,
    CMD_SUBSCRIBE = 0x30,
    CMD_UNSUBSCRIBE = 0x31,
    CMD_NOTIFY = 0xF0,
    CMD_ERROR = 0xFF,
  };

  /** @brief Status codes returned in error responses */

  enum
  {
    STATUS_OK = 0x00,
    STATUS_INVALID_CMD = 0x01,
    STATUS_INVALID_OBJ = 0x02,
    STATUS_INVALID_CFG = 0x03,
    STATUS_READ_ONLY = 0x04,
    STATUS_WRITE_ONLY = 0x05,
    STATUS_INVALID_FORMAT = 0x06,
    STATUS_ERROR = 0xFF,
  };

  /* @brief I/O direction codes returned by the GET_INFO response */

  enum
  {
    IO_TYPE_READ_ONLY = 0x01,
    IO_TYPE_WRITE_ONLY = 0x02,
    IO_TYPE_READ_WRITE = 0x03,
  };

  /** @brief Configuration for binding an I/O object to a simple protocol */

  struct
  {
    SObjectId::ObjectId objid;
  } typedef SProtoSimpleIOBind;

  /**
   * @brief Constructor.
   *
   * @param desc Device descriptor containing protocol configuration.
   */

  explicit CProtoSimpleBase(CDescObject &desc)
    : CProtoCommon(desc)
    , initialized(false)
  {
  }

protected:
  /** @brief Internal I/O data mapping for simple protocol handlers */

  struct
  {
    const SProtoSimpleIOBind *cfg;
    io_ddata_t *iodata;
  } typedef SProtoSimpleData;

#ifdef CONFIG_DAWN_IO_NOTIFY
  /** @brief Notification context for subscribed IOs */

  struct
  {
    SObjectId::ObjectId objid;
    CIOCommon *io;
    CProtoSimpleBase *proto;
  } typedef SProtoSimpleNotify;
#endif

  uint8_t rxbuffer[FRAME_MAX_PAYLOAD + FRAME_MIN_LEN]; //< Shared receive frame buffer
  bool initialized;                                    //< Initialization completion flag
  std::vector<SProtoSimpleIOBind *> iobinds; //< Vector of I/O bind configurations (from descriptor)
  std::vector<SProtoSimpleData *> iobuffers; //< Vector of runtime I/O data structures

#ifdef CONFIG_DAWN_IO_NOTIFY
  std::mutex subsMutex;                      //< Mutex for subscription set access
  std::set<SObjectId::ObjectId> subscriptions;      //< Set of subscribed IO object IDs
  std::vector<SProtoSimpleNotify *> notifyContexts; //< Vector of notification callback contexts
#endif

  /**
   * @brief Calculate 16-bit CRC checksum.
   *
   * @param data Buffer to checksum.
   * @param len Number of bytes.
   * @return CRC16 value.
   */

  uint16_t calculateCrc(const uint8_t *data, size_t len);

  /**
   * @brief Process a received frame.
   *
   * @param frame Frame buffer pointer.
   * @param len Frame length in bytes.
   * @return 0 on success, negative errno on error.
   */

  int handleFrame(const uint8_t *frame, size_t len);

  /**
   * @brief Store an allocated IO binding.
   *
   * @param cfg Binding configuration entry.
   */

  void allocObject(SProtoSimpleIOBind *cfg);

  /**
   * @brief Allocate shared per-IO data buffers.
   *
   * @return 0 on success, negative errno on failure.
   */

  int createBuffers();

  /**
   * @brief Destroy shared per-IO data buffers.
   *
   * @return 0 on success, negative errno on failure.
   */

  int destroyBuffers();

#ifdef CONFIG_DAWN_IO_NOTIFY
  /**
   * @brief Setup notification callbacks for notify-capable IOs.
   *
   * @return 0 on success, negative errno on failure.
   */

  int setupNotifications();

  /**
   * @brief Clear the active subscription set.
   */

  void cleanupNotifications();

  /**
   * @brief Destroy all notification callback contexts.
   */

  void destroyNotifications();

  /**
   * @brief Notification callback for IO data events.
   *
   * @param priv Private context (SProtoSimpleNotify*).
   * @param data IO data buffer.
   * @return 0 on success, negative errno on failure.
   */

  static int notifierCb(void *priv, io_ddata_t *data);
#endif

  /**
   * @brief Send a framed response via the derived transport.
   *
   * @param cmd Response command ID.
   * @param payload Optional payload data.
   * @param len Payload length.
   * @return 0 on success, negative errno on error.
   */

  virtual int sendFrame(uint8_t cmd, const uint8_t *payload, size_t len) = 0;

private:
  /**
   * @brief Send an error response frame.
   *
   * @param error_code Status code (STATUS_*).
   * @param context Additional error context.
   */

  io_ddata_t *findIOBuffer(SObjectId::ObjectId objid);

  void sendError(uint8_t error_code, uint8_t context);

  /** @brief Handle PING command */

  void cmdPing();

  /**
   * @brief Handle GET_IO command.
   *
   * @param payload Command payload buffer.
   * @param len Payload length.
   */

  void cmdGetIO(const uint8_t *payload, size_t len);

  /**
   * @brief Handle SET_IO command.
   *
   * @param payload Command payload (I/O ID + data).
   * @param len Payload length.
   */

  void cmdSetIO(const uint8_t *payload, size_t len);

  /**
   * @brief Handle GET_IO_SEEK command.
   *
   * @param payload Command payload buffer.
   * @param len Payload length.
   */

  void cmdGetIOSeek(const uint8_t *payload, size_t len);

  /**
   * @brief Handle SET_IO_SEEK command.
   *
   * Payload format: [objid:4][offset:4][data:n].
   */

  void cmdSetIOSeek(const uint8_t *payload, size_t len);

  /**
   * @brief Handle GET_CFG command.
   *
   * @param payload Command payload (config ID).
   * @param len Payload length.
   */

  void cmdGetCfg(const uint8_t *payload, size_t len);

  /**
   * @brief Handle SET_CFG command.
   *
   * @param payload Command payload (config ID + data).
   * @param len Payload length.
   */

  void cmdSetCfg(const uint8_t *payload, size_t len);

  /**
   * @brief Handle GET_INFO command.
   *
   * @param payload Command payload.
   * @param len Payload length.
   */

  void cmdGetInfo(const uint8_t *payload, size_t len);

  /**
   * @brief Handle LIST_IOS command.
   */

  void cmdListIOs();

#ifdef CONFIG_DAWN_IO_NOTIFY
  /**
   * @brief Handle SUBSCRIBE command.
   *
   * @param payload Command payload (objid).
   * @param len Payload length.
   */

  void cmdSubscribe(const uint8_t *payload, size_t len);

  /**
   * @brief Handle UNSUBSCRIBE command.
   *
   * @param payload Command payload (objid).
   * @param len Payload length.
   */

  void cmdUnsubscribe(const uint8_t *payload, size_t len);
#endif
};
} // Namespace dawn
