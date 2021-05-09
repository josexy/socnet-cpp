#ifndef SOC_NET_TCPCONNECTION_H
#define SOC_NET_TCPCONNECTION_H

#include <functional>

#include "../../utility/include/MacroCtl.h"
#include "../include/ServerSocket.h"
#include "Buffer.h"

#ifdef SUPPORT_SSL_LIB
#include "SSL.h"
#endif

namespace soc {
namespace net {

class TcpConnection;
using TcpConnectionPtr = TcpConnection *;
class TcpConnection {
public:
#ifdef SUPPORT_SSL_LIB
  SSL *ssl() const { return ssl_; }
  void set_ssl(SSL *ssl) { ssl_ = ssl; }
#endif

  void initialize(int connfd);

  int fd() const noexcept { return connfd_; }
  InetAddress local_address() { return ServerSocket::sockname(connfd_); }
  InetAddress peer_address() { return ServerSocket::peername(connfd_); }

  int read(int *err);
  int write(int *err);

  bool disconnected() const noexcept { return disconnected_; }
  void disconnected(bool is) { disconnected_ = is; }

  bool keep_alive() const noexcept { return keep_alive_; }
  void keep_alive(bool is) { keep_alive_ = is; }

  void *context() { return context_; }
  void context(void *context) { context_ = context; }

  decltype(auto) sender() { return &sender_; }
  decltype(auto) recver() { return &recver_; }

private:
  int connfd_;

#ifdef SUPPORT_SSL_LIB
  SSL *ssl_;
#endif

  bool disconnected_;
  bool keep_alive_;

  void *context_;
  Buffer sender_;
  Buffer recver_;
};
} // namespace net
} // namespace soc

#endif
