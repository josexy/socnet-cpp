#include "../include/TcpConnection.h"

#include <unistd.h>

using namespace soc::net;

void TcpConnection::initialize(int connfd) {
  sender_.retired_all();
  recver_.retired_all();

  connfd_ = connfd;
  disconnected_ = keep_alive_ = false;
  context_ = nullptr;
  ssl_ = nullptr;
}

int TcpConnection::read(int *err) {
  int n = -1;
  if (ssl_) {
    char buffer[kBufferSize];
    while ((n = ::SSL_read(ssl_, buffer, kBufferSize)) > 0) {
      recver_.append(buffer, n);
    }
    *err = ::SSL_get_error(ssl_, n);
  } else {
    while ((n = recver_.read_fd(connfd_, err)) > 0) {
      // while ((n = ::read(connfd_, buffer, kBufferSize)) > 0) {
      // recver_.append(buffer, n);
    }
    // *err = errno;
  }
  return n;
}

int TcpConnection::write(int *err) {
  size_t len = sender_.readable();
  int r_len = len;
  int n = -1;
  while (true) {
    if (ssl_) {
      n = ::SSL_write(ssl_, sender_.peek() + len - r_len, r_len);
      if (n < 0) {
        *err = ::SSL_get_error(ssl_, n);
        return n;
      }
    } else {
      n = ::write(connfd_, sender_.peek() + len - r_len, r_len);
      if (n < 0) {
        *err = errno;
        return n;
      }
    }
    sender_.retired(n);
    r_len -= n;
    if (r_len <= 0) {
      n = 0;
      sender_.retired_all();
      break;
    }
  }
  return n;
}
