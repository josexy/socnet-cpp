#ifndef SOC_NET_TCPCONNECTION_H
#define SOC_NET_TCPCONNECTION_H

#include "../include/ServerSocket.h"
#include "Channel.h"
#include <memory>

namespace soc {
namespace net {

class TcpConnection {
public:
  typedef struct {
    int n;
    int err;
  } channel_status_ne;

  typedef struct {
    int n;
    bool again;
  } channel_status_na;

  typedef struct {
    int n;
    int err;
    bool completed;
  } channel_status_nec;

  TcpConnection();

  void initialize(int connfd);

  int getFd() const noexcept { return connfd_; }
  InetAddress getLocalAddr() { return option::getSockName(connfd_); }
  InetAddress getPeerAddr() { return option::getPeerName(connfd_); }

  channel_status_ne read();
  channel_status_nec write();

  channel_status_na readAgain();
  channel_status_nec writeAgain();

  Channel *getChannel() const noexcept { return channel_.get(); }
  void setChannel(Channel *channel) { channel_.reset(channel); }

  bool isDisconnected() const noexcept { return disconnected_; }
  void setDisconnected(bool is) { disconnected_ = is; }

  bool isKeepAlive() const noexcept { return keep_alive_; }
  void setKeepAlive(bool is) { keep_alive_ = is; }

  void *getContext() const noexcept { return context_; }
  void setContext(void *context) { context_ = context; }

  Buffer *getSender() { return channel_->getSender(); }
  Buffer *getRecver() { return channel_->getRecver(); }

private:
  int connfd_;
  bool disconnected_;
  bool keep_alive_;
  void *context_;
  std::shared_ptr<Channel> channel_;
};
} // namespace net
} // namespace soc

#endif
