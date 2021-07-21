#ifndef SOC_NET_TCPCONNECTION_H
#define SOC_NET_TCPCONNECTION_H

#include "../include/ServerSocket.h"
#include "Channel.h"
#include <memory>

namespace soc {
namespace net {

static constexpr const size_t kBufferSize = 4096;

class TcpConnection {
public:
  TcpConnection();

  void initialize(int connfd);

  int fd() const noexcept { return connfd_; }
  InetAddress localAddr() { return option::sockname(connfd_); }
  InetAddress peerAddr() { return option::peername(connfd_); }

  std::pair<int, int> read();
  std::pair<int, int> write();

  std::pair<int, bool> readAgain();
  std::pair<int, bool> writeAgain();

  Channel *channel() const noexcept { return channel_.get(); }
  void setChannel(Channel *channel) { channel_.reset(channel); }

  bool disconnected() const noexcept { return disconnected_; }
  void setDisconnected(bool is) { disconnected_ = is; }

  bool keepAlive() const noexcept { return keep_alive_; }
  void setKeepAlive(bool is) { keep_alive_ = is; }

  void *context() const noexcept { return context_; }
  void setContext(void *context) { context_ = context; }

  decltype(auto) sender() { return &sender_; }
  decltype(auto) recver() { return &recver_; }

private:
  int connfd_;
  bool disconnected_;
  bool keep_alive_;
  void *context_;
  std::shared_ptr<Channel> channel_;
  Buffer sender_;
  Buffer recver_;
};
} // namespace net
} // namespace soc

#endif
