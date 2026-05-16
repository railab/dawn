// dawn/src/proto/wakaama/transport.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "internal.hxx"

#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>

#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
extern "C"
{
#  include <tinydtls/dtls.h>
}
#endif

namespace dawn
{
namespace wakaama_internal
{
/**
 * @brief UDP and optional DTLS transport used by the Wakaama client runtime.
 */

class Transport
{
public:
  /** @brief Runtime state for one remote LwM2M server connection. */

  struct Connection
  {
    Connection *next;
    int sock;
    uint16_t securityInstanceId;
    const uint8_t *pskIdentity;
    size_t pskIdentityLen;
    const uint8_t *pskKey;
    size_t pskKeyLen;
    sockaddr_storage addr;
    socklen_t addrLen;
#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
    session_t dtlsSession;
    dtls_context_t *dtlsContext;
    time_t lastSend;
    bool dtls;
#endif
  };

  /** @brief Construct transport state for a Wakaama protocol owner. */

  explicit Transport(CProtoWakaama &owner);

  /** @brief Close sockets, DTLS state, and connection-pool allocations. */

  ~Transport();

  /** @brief Allocate the fixed connection pool used by Wakaama callbacks. */

  int initConnectionPool();

  /** @brief Open and bind the local UDP socket. */

  int openSocket();

  /** @brief Initialize optional shared DTLS context state. */

  int initDtls();

  /** @brief Destroy optional shared DTLS context state. */

  void destroyDtls();

  /** @brief Close every active server connection. */

  void closeAllConnections();

  /** @brief Run the transport receive loop. */

  void thread();

  /** @brief Connect to the server referenced by a Security object instance. */

  Connection *connectServer(uint16_t securityInstanceId);

  /** @brief Find an active connection by peer address. */

  Connection *findConnection(const void *addr, size_t addrLen);

  /** @brief Deliver one received packet into the Wakaama runtime. */

  void handlePacket(Connection *conn, uint8_t *buffer, size_t length);

  /** @brief Close one active server connection. */

  void closeConnection(Connection *conn);

private:
  /** @brief Receive and dispatch a single datagram from the local socket. */

  bool receiveOne(uint8_t *buffer, size_t capacity);

  CProtoWakaama &owner;
  Connection *connections;
  std::vector<Connection *> connectionPool;
  int sockfd;
#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
  dtls_context_t *dtlsContext;
  dtls_handler_t dtlsHandler;
#endif
};

} // namespace wakaama_internal
} // namespace dawn
