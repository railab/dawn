// dawn/src/proto/wakaama/transport.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/wakaama/wakaama.hxx"
#include "client.hxx"
#include "transport.hxx"

extern "C"
{
#include <liblwm2m.h>
}

#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
extern "C"
{
#  include <tinydtls/dtls.h>
#  include <tinydtls/tinydtls.h>
}
#endif

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <new>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace dawn;
using namespace dawn::wakaama_internal;

#if defined(CONFIG_ARCH_SIM) && defined(CONFIG_SIM_NETUSRSOCK)
#  define DAWN_WAKAAMA_POLL_RECV 1
extern "C" void host_usrsock_loop(void);
#endif

namespace
{
void resetConnection(Transport::Connection *conn);
bool parseServerUri(const char *uri, char *host, size_t hostSize, uint16_t *port, uint8_t *scheme);
bool sockaddrEqual(const sockaddr_storage &lhs,
                   socklen_t lhsLen,
                   const sockaddr_storage &rhs,
                   socklen_t rhsLen);
#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
int dtlsSendToPeer(dtls_context_t *ctx, session_t *session, uint8 *data, size_t len);
int dtlsReadFromPeer(dtls_context_t *ctx, session_t *session, uint8 *data, size_t len);
int dtlsGetPskInfo(dtls_context_t *ctx,
                   const session_t *session,
                   dtls_credentials_type_t type,
                   const unsigned char *id,
                   size_t idLen,
                   unsigned char *result,
                   size_t resultLen);
#endif
} // namespace

Transport::Transport(CProtoWakaama &owner_)
  : owner(owner_)
  , connections(nullptr)
  , sockfd(-1)
#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
  , dtlsContext(nullptr)
  , dtlsHandler()
#endif
{
}

Transport::~Transport()
{
  destroyDtls();
  closeAllConnections();

  for (Connection *conn : connectionPool)
    {
      delete conn;
    }

  connectionPool.clear();

  if (sockfd >= 0)
    {
      close(sockfd);
      sockfd = -1;
    }
}

int Transport::initConnectionPool()
{
  const size_t capacity = owner.serverPoolCapacity();

  connectionPool.reserve(capacity);
  for (size_t i = 0; i < capacity; i++)
    {
      Connection *conn = new (std::nothrow) Connection();
      if (conn == nullptr)
        {
          return -ENOMEM;
        }

      resetConnection(conn);
      connectionPool.push_back(conn);
    }

  return OK;
}

int Transport::openSocket()
{
  sockaddr_in addr;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      return -errno;
    }

  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(owner.localPort);
  if (bind(sockfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
      int err = errno;
      close(sockfd);
      sockfd = -1;
      return -err;
    }

  return OK;
}

int Transport::initDtls()
{
#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
  for (const CProtoWakaama::ServerConfig &server : owner.servers)
    {
      if (server.scheme == CProtoWakaama::WAKAAMA_SERVER_SCHEME_COAPS)
        {
          dtls_init();
          dtlsContext = dtls_new_context(this);
          if (dtlsContext == nullptr)
            {
              return -ENOMEM;
            }

          std::memset(&dtlsHandler, 0, sizeof(dtlsHandler));
          dtlsHandler.write = dtlsSendToPeer;
          dtlsHandler.read = dtlsReadFromPeer;
          dtlsHandler.get_psk_info = dtlsGetPskInfo;
          dtls_set_handler(dtlsContext, &dtlsHandler);
          break;
        }
    }

  if (dtlsContext != nullptr)
    {
      for (const CProtoWakaama::ServerConfig &server : owner.servers)
        {
          if (server.scheme == CProtoWakaama::WAKAAMA_SERVER_SCHEME_COAPS &&
              connectServer(server.securityInstanceId) == nullptr)
            {
              return -EIO;
            }
        }
    }
#endif

  return OK;
}

void Transport::destroyDtls()
{
#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
  if (dtlsContext != nullptr)
    {
      dtls_free_context(dtlsContext);
      dtlsContext = nullptr;
    }
#endif
}

Transport::Connection *Transport::connectServer(uint16_t securityInstanceId)
{
  char host[256];
  sockaddr_in serverAddr;
  const CProtoWakaama::ServerConfig *server;
  security_instance_s *secInst;
  Connection *conn;
  uint16_t serverPortValue;
  uint8_t scheme;

  if (sockfd < 0)
    {
      return nullptr;
    }

  for (conn = connections; conn != nullptr; conn = conn->next)
    {
      if (conn->securityInstanceId == securityInstanceId)
        {
          return conn;
        }
    }

  server = owner.findServer(securityInstanceId);
  secInst =
    owner.runtime == nullptr ? nullptr : owner.runtime->findSecurityInstance(securityInstanceId);

  if (server != nullptr)
    {
      std::snprintf(host, sizeof(host), "%s", server->host.c_str());
      serverPortValue = server->port;
      scheme = server->scheme;
    }
  else if (secInst != nullptr &&
           parseServerUri(secInst->uri, host, sizeof(host), &serverPortValue, &scheme))
    {
      if (scheme == CProtoWakaama::WAKAAMA_SERVER_SCHEME_COAPS)
        {
          DAWNERR("Wakaama bootstrap-created coaps server is not preallocated\n");
          return nullptr;
        }
    }
  else
    {
      return nullptr;
    }

  std::memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(serverPortValue);
  if (inet_pton(AF_INET, host, &serverAddr.sin_addr) != 1)
    {
      DAWNERR("Wakaama server host must be a numeric IPv4 address: %s\n", host);
      return nullptr;
    }

  conn = nullptr;
  for (Connection *slot : connectionPool)
    {
      if (slot->sock < 0)
        {
          conn = slot;
          break;
        }
    }

  if (conn == nullptr)
    {
      return nullptr;
    }

  resetConnection(conn);
  conn->sock = sockfd;
  conn->securityInstanceId = securityInstanceId;
  if (server != nullptr)
    {
      conn->pskIdentity = reinterpret_cast<const uint8_t *>(server->pskIdentity.data());
      conn->pskIdentityLen = server->pskIdentity.size();
      conn->pskKey = server->pskKey.data();
      conn->pskKeyLen = server->pskKey.size();
    }
  else if (secInst != nullptr)
    {
      conn->pskIdentity = secInst->publicIdentity;
      conn->pskIdentityLen = secInst->publicIdentityLen;
      conn->pskKey = secInst->secretKey;
      conn->pskKeyLen = secInst->secretKeyLen;
    }
  std::memcpy(&conn->addr, &serverAddr, sizeof(serverAddr));
  conn->addrLen = sizeof(serverAddr);
#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
  conn->dtls = scheme == CProtoWakaama::WAKAAMA_SERVER_SCHEME_COAPS;
  if (conn->dtls)
    {
      if (dtlsContext == nullptr)
        {
          resetConnection(conn);
          return nullptr;
        }

      std::memset(&conn->dtlsSession, 0, sizeof(conn->dtlsSession));
      std::memcpy(&conn->dtlsSession.addr.st, &conn->addr, conn->addrLen);
      conn->dtlsSession.size = conn->addrLen;
      conn->dtlsContext = dtlsContext;
      conn->lastSend = lwm2m_gettime();
      if (dtls_connect(conn->dtlsContext, &conn->dtlsSession) != 0)
        {
          resetConnection(conn);
          return nullptr;
        }
    }
#endif
  conn->next = connections;
  connections = conn;

  return conn;
}

void Transport::closeConnection(Connection *conn)
{
  if (conn == nullptr)
    {
      return;
    }

  Connection **link = &connections;
  while (*link != nullptr)
    {
      if (*link == conn)
        {
          *link = conn->next;
#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
          if (conn->dtls && conn->dtlsContext != nullptr)
            {
              dtls_peer_t *peer = dtls_get_peer(conn->dtlsContext, &conn->dtlsSession);
              if (peer != nullptr)
                {
                  dtls_reset_peer(conn->dtlsContext, peer);
                }
            }
#endif
          resetConnection(conn);
          return;
        }

      link = &(*link)->next;
    }
}

Transport::Connection *Transport::findConnection(const void *addr, size_t addrLen)
{
  const sockaddr_storage *peer = static_cast<const sockaddr_storage *>(addr);

  if (peer == nullptr)
    {
      return nullptr;
    }

  for (Connection *conn = connections; conn != nullptr; conn = conn->next)
    {
      if (sockaddrEqual(conn->addr, conn->addrLen, *peer, static_cast<socklen_t>(addrLen)))
        {
          return conn;
        }
    }

  return nullptr;
}

void Transport::handlePacket(Connection *conn, uint8_t *buffer, size_t length)
{
  if (owner.runtime != nullptr && conn != nullptr)
    {
      owner.runtime->handlePacket(buffer, length, conn);
    }
}

void Transport::closeAllConnections()
{
  while (connections != nullptr)
    {
      closeConnection(connections);
    }

  for (Connection *conn : connectionPool)
    {
      resetConnection(conn);
    }

  connections = nullptr;
}

bool Transport::receiveOne(uint8_t *buffer, size_t capacity)
{
  sockaddr_storage addr;
  socklen_t addrLen = sizeof(addr);
  ssize_t len;

  len = recvfrom(sockfd, buffer, capacity, 0, reinterpret_cast<sockaddr *>(&addr), &addrLen);
  if (len > 0)
    {
      Connection *conn = connections;

      while (conn != nullptr)
        {
          if (sockaddrEqual(conn->addr, conn->addrLen, addr, addrLen))
            {
              break;
            }

          conn = conn->next;
        }

      if (conn != nullptr)
        {
#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
          if (conn->dtls)
            {
              dtls_handle_message(
                conn->dtlsContext, &conn->dtlsSession, buffer, static_cast<size_t>(len));
            }
          else
#endif
            {
              owner.runtime->handlePacket(buffer, static_cast<size_t>(len), conn);
            }
        }
      else
        {
          DAWNWARN("Wakaama packet from unknown peer\n");
        }

      return true;
    }

  if (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
      DAWNERR("Wakaama recvfrom failed: %d\n", errno);
    }

  return false;
}

void Transport::thread()
{
  uint8_t buffer[CProtoWakaama::RX_BUFFER_SIZE];

  while (!owner.workerThread().shouldQuit())
    {
      time_t timeout = 1;
      int ret;

#ifdef CONFIG_DAWN_IO_NOTIFY
      owner.processChangedResources();
#endif

      ret = owner.runtime == nullptr ? -ENODEV : owner.runtime->step(&timeout);
      if (ret != 0)
        {
          DAWNERR("lwm2m_step failed: %d\n", ret);
          break;
        }

#ifdef DAWN_WAKAAMA_POLL_RECV
      host_usrsock_loop();
#endif

      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(sockfd, &readfds);

      timeval tv;
      tv.tv_sec = timeout > 0 ? timeout : 1;
      tv.tv_usec = 0;
#ifdef DAWN_WAKAAMA_POLL_RECV
      if (tv.tv_sec > 0)
        {
          tv.tv_sec = 0;
          tv.tv_usec = 100000;
        }
#endif

      ret = select(sockfd + 1, &readfds, nullptr, nullptr, &tv);
      if (ret > 0 && FD_ISSET(sockfd, &readfds))
        {
          receiveOne(buffer, sizeof(buffer));
        }
      else if (ret < 0)
        {
          DAWNERR("Wakaama select failed: %d\n", errno);
        }

#ifdef DAWN_WAKAAMA_POLL_RECV
      host_usrsock_loop();
#endif
    }

  owner.workerThread().markThreadFinished();
}

int CProtoWakaama::initConnectionPool()
{
  if (transport == nullptr)
    {
      transport = new (std::nothrow) Transport(*this);
      if (transport == nullptr)
        {
          return -ENOMEM;
        }
    }

  return transport->initConnectionPool();
}

int CProtoWakaama::openSocket()
{
  return transport == nullptr ? -ENODEV : transport->openSocket();
}

int CProtoWakaama::initDtls()
{
  return transport == nullptr ? -ENODEV : transport->initDtls();
}

void CProtoWakaama::destroyDtls()
{
  if (transport != nullptr)
    {
      transport->destroyDtls();
    }
}

void CProtoWakaama::closeAllConnections()
{
  if (transport != nullptr)
    {
      transport->closeAllConnections();
    }
}

void CProtoWakaama::destroyConnectionPool()
{
  delete transport;
  transport = nullptr;
}

void CProtoWakaama::thread()
{
  if (transport != nullptr)
    {
      transport->thread();
    }
}

extern "C" uint8_t lwm2m_buffer_send(void *sessionH, uint8_t *buffer, size_t length, void *userdata)
{
  Transport::Connection *conn = static_cast<Transport::Connection *>(sessionH);

  UNUSED(userdata);

  if (conn == nullptr)
    {
      return COAP_500_INTERNAL_SERVER_ERROR;
    }

#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
  if (conn->dtls)
    {
      if (dtls_write(conn->dtlsContext, &conn->dtlsSession, buffer, length) < 0)
        {
          return COAP_500_INTERNAL_SERVER_ERROR;
        }

      return COAP_NO_ERROR;
    }
#endif

  if (sendto(
        conn->sock, buffer, length, 0, reinterpret_cast<sockaddr *>(&conn->addr), conn->addrLen) <
      0)
    {
      return COAP_500_INTERNAL_SERVER_ERROR;
    }

  return COAP_NO_ERROR;
}

extern "C" bool lwm2m_session_is_equal(void *session1, void *session2, void *userData)
{
  UNUSED(userData);

  return session1 == session2;
}

extern "C" void *lwm2m_connect_server(uint16_t secObjInstID, void *userData)
{
  Transport *transport = static_cast<Transport *>(userData);

  return transport == nullptr ? nullptr : transport->connectServer(secObjInstID);
}

extern "C" void lwm2m_close_connection(void *sessionH, void *userData)
{
  Transport *transport = static_cast<Transport *>(userData);
  Transport::Connection *conn = static_cast<Transport::Connection *>(sessionH);

  if (transport != nullptr && conn != nullptr)
    {
      transport->closeConnection(conn);
    }
}

namespace
{
void resetConnection(Transport::Connection *conn)
{
  if (conn != nullptr)
    {
      std::memset(conn, 0, sizeof(*conn));
      conn->sock = -1;
    }
}

#ifdef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
int dtlsSendToPeer(dtls_context_t *ctx, session_t *session, uint8 *data, size_t len)
{
  Transport *transport = static_cast<Transport *>(ctx->app);
  Transport::Connection *conn =
    transport == nullptr ? nullptr : transport->findConnection(&session->addr.st, session->size);

  if (conn == nullptr)
    {
      return -1;
    }

  if (sendto(conn->sock, data, len, 0, reinterpret_cast<sockaddr *>(&conn->addr), conn->addrLen) <
      0)
    {
      return -1;
    }

  conn->lastSend = lwm2m_gettime();
  return static_cast<int>(len);
}

int dtlsReadFromPeer(dtls_context_t *ctx, session_t *session, uint8 *data, size_t len)
{
  Transport *transport = static_cast<Transport *>(ctx->app);
  Transport::Connection *conn =
    transport == nullptr ? nullptr : transport->findConnection(&session->addr.st, session->size);

  if (conn == nullptr)
    {
      return -1;
    }

  transport->handlePacket(conn, data, len);
  return 0;
}

int dtlsGetPskInfo(dtls_context_t *ctx,
                   const session_t *session,
                   dtls_credentials_type_t type,
                   const unsigned char *id,
                   size_t idLen,
                   unsigned char *result,
                   size_t resultLen)
{
  Transport *transport = static_cast<Transport *>(ctx->app);
  Transport::Connection *conn =
    transport == nullptr ? nullptr : transport->findConnection(&session->addr.st, session->size);
  const uint8_t *value = nullptr;
  size_t valueLen = 0;

  UNUSED(id);
  UNUSED(idLen);

  if (conn == nullptr)
    {
      return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }

  switch (type)
    {
      case DTLS_PSK_IDENTITY:
        value = conn->pskIdentity;
        valueLen = conn->pskIdentityLen;
        break;

      case DTLS_PSK_KEY:
        value = conn->pskKey;
        valueLen = conn->pskKeyLen;
        break;

      case DTLS_PSK_HINT:
        return 0;

      default:
        return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }

  if (value == nullptr || valueLen == 0 || resultLen < valueLen)
    {
      return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }

  std::memcpy(result, value, valueLen);
  return static_cast<int>(valueLen);
}
#endif

bool parseServerUri(const char *uri, char *host, size_t hostSize, uint16_t *port, uint8_t *scheme)
{
  const char *begin;
  const char *portSep;
  size_t hostLen;
  unsigned long parsedPort;
  char *end;

  if (uri == nullptr || host == nullptr || hostSize == 0 || port == nullptr || scheme == nullptr)
    {
      return false;
    }

  if (std::strncmp(uri, "coaps://", 8) == 0)
    {
      begin = uri + 8;
      *scheme = CProtoWakaama::WAKAAMA_SERVER_SCHEME_COAPS;
    }
  else if (std::strncmp(uri, "coap://", 7) == 0)
    {
      begin = uri + 7;
      *scheme = CProtoWakaama::WAKAAMA_SERVER_SCHEME_COAP;
    }
  else
    {
      return false;
    }

  portSep = std::strrchr(begin, ':');
  if (portSep == nullptr)
    {
      return false;
    }

  hostLen = static_cast<size_t>(portSep - begin);
  if (hostLen == 0 || hostLen >= hostSize)
    {
      return false;
    }

  if (hostLen >= 2 && begin[0] == '[' && begin[hostLen - 1] == ']')
    {
      begin++;
      hostLen -= 2;
    }

  if (hostLen == 0)
    {
      return false;
    }

  std::memcpy(host, begin, hostLen);
  host[hostLen] = '\0';

  parsedPort = std::strtoul(portSep + 1, &end, 10);
  if (*end != '\0' || parsedPort == 0 || parsedPort > UINT16_MAX)
    {
      return false;
    }

  *port = static_cast<uint16_t>(parsedPort);
  return true;
}

bool sockaddrEqual(const sockaddr_storage &lhs,
                   socklen_t lhsLen,
                   const sockaddr_storage &rhs,
                   socklen_t rhsLen)
{
  if (lhs.ss_family != rhs.ss_family)
    {
      return false;
    }

  switch (lhs.ss_family)
    {
      case AF_INET:
        {
          const sockaddr_in *lhs4 = reinterpret_cast<const sockaddr_in *>(&lhs);
          const sockaddr_in *rhs4 = reinterpret_cast<const sockaddr_in *>(&rhs);

          return lhsLen >= sizeof(sockaddr_in) && rhsLen >= sizeof(sockaddr_in) &&
                 lhs4->sin_port == rhs4->sin_port && lhs4->sin_addr.s_addr == rhs4->sin_addr.s_addr;
        }

#ifdef AF_INET6
      case AF_INET6:
        {
          const sockaddr_in6 *lhs6 = reinterpret_cast<const sockaddr_in6 *>(&lhs);
          const sockaddr_in6 *rhs6 = reinterpret_cast<const sockaddr_in6 *>(&rhs);

          return lhsLen >= sizeof(sockaddr_in6) && rhsLen >= sizeof(sockaddr_in6) &&
                 lhs6->sin6_port == rhs6->sin6_port && lhs6->sin6_scope_id == rhs6->sin6_scope_id &&
                 std::memcmp(&lhs6->sin6_addr, &rhs6->sin6_addr, sizeof(lhs6->sin6_addr)) == 0;
        }
#endif

      default:
        {
          return lhsLen == rhsLen && std::memcmp(&lhs, &rhs, lhsLen) == 0;
        }
    }
}
} // namespace
