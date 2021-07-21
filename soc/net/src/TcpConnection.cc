#include "../include/TcpConnection.h"

using namespace soc::net;

TcpConnection::TcpConnection()
    : disconnected_(false), keep_alive_(false), context_(nullptr),
      channel_(nullptr) {}

void TcpConnection::initialize(int connfd) {
  sender_.retiredAll();
  recver_.retiredAll();

  connfd_ = connfd;
  disconnected_ = keep_alive_ = false;
  context_ = nullptr;
  channel_ = nullptr;
}

std::pair<int, int> TcpConnection::read() {
  int n = -1;
  char buffer[kBufferSize];
  while ((n = channel_->read(buffer, kBufferSize)) > 0) {
    recver_.append(buffer, n);
  }

  return std::make_pair(n, channel_->getError(n));
}

std::pair<int, int> TcpConnection::write() {
  size_t len = sender_.readable();
  int r_len = len;
  int n = -1;

  while (true) {
    n = channel_->write(sender_.peek() + len - r_len, r_len);
    if (n < 0)
      break;

    sender_.retired(n);
    r_len -= n;
    if (r_len <= 0) {
      n = 0;
      sender_.retiredAll();
      break;
    }
  }
  return std::make_pair(n, channel_->getError(n));
}

std::pair<int, bool> TcpConnection::readAgain() {
  const auto [n, err] = read();
  bool again = false;
  if (channel_->type() == ChannelType::Raw)
    again = (err == EAGAIN);
  else if (channel_->type() == ChannelType::Ssl)
    again = (err == SSL_ERROR_WANT_READ);
  return std::make_pair(n, again);
}

std::pair<int, bool> TcpConnection::writeAgain() {
  const auto [n, err] = write();
  bool again = false;
  if (channel_->type() == ChannelType::Raw)
    again = (err == EAGAIN);
  else if (channel_->type() == ChannelType::Ssl)
    again = (err == SSL_ERROR_WANT_WRITE);
  return std::make_pair(n, again);
}
