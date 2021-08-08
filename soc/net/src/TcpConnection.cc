#include "../include/TcpConnection.h"

using namespace soc::net;

TcpConnection::TcpConnection()
    : disconnected_(false), keep_alive_(false), context_(nullptr),
      channel_(nullptr) {}

void TcpConnection::initialize(int connfd) {
  sender_.reset();
  recver_.reset();

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
  bool switch_mapping = sender_.isSwitchMapping();
  int r_len = sender_.readable();
  int n = -1;

  while (true) {
    n = channel_->write(sender_.beginRead(), r_len);
    if (n < 0)
      break;

    sender_.retired(n);
    r_len -= n;
    if (r_len <= 0) {
      n = 0;
      if (!switch_mapping && sender_.mappingAddr()) {
        switch_mapping = true;
        sender_.switchMapping(switch_mapping);
        r_len = sender_.mappingSize();
        sender_.hasWritten(r_len);
      } else {
        // All data transmission completed
        sender_.reset();
        break;
      }
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
