#include "../include/TcpConnection.h"

using namespace soc::net;

TcpConnection::TcpConnection()
    : disconnected_(false), keep_alive_(false), context_(nullptr),
      channel_(nullptr) {}

void TcpConnection::initialize(int connfd) {
  connfd_ = connfd;
  disconnected_ = keep_alive_ = false;
  context_ = nullptr;
  channel_ = nullptr;
}

TcpConnection::channel_status_ne TcpConnection::read() {
  int n = -1;
  while ((n = channel_->read()) > 0)
    ;
  return {n, channel_->getError(n)};
}

TcpConnection::channel_status_nec TcpConnection::write() {
  int n = -1;
  bool cflag = false;
  while (true) {
    n = channel_->write(&cflag);
    if (n < 0)
      break;
    if (cflag == true) {
      n = 0;
      // All data transmission completed
      channel_->reset();
      break;
    }
  }
  return {n, channel_->getError(n), cflag};
}

TcpConnection::channel_status_na TcpConnection::readAgain() {
  const auto [n, err] = read();
  bool again = false;
  if (channel_->getType() == ChannelType::Raw)
    again = (err == EAGAIN);
  else if (channel_->getType() == ChannelType::Ssl)
    again = (err == SSL_ERROR_WANT_READ);
  return {n, again};
}

TcpConnection::channel_status_nec TcpConnection::writeAgain() {
  const auto [n, err, cflag] = write();
  bool again = false;
  if (channel_->getType() == ChannelType::Raw)
    again = (err == EAGAIN);
  else if (channel_->getType() == ChannelType::Ssl)
    again = (err == SSL_ERROR_WANT_WRITE);
  return {n, err, cflag};
}
