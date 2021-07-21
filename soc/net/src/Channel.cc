#include "../include/Channel.h"
using namespace soc::net;

int SslChannel::read(void *data, size_t n) { return ::SSL_read(ssl_, data, n); }

int SslChannel::write(const void *data, size_t n) {
  return ::SSL_write(ssl_, data, n);
}

void SslChannel::close() {
  ::SSL_shutdown(ssl_);
  ::SSL_free(ssl_);
  ssl_ = nullptr;
}

int SslChannel::getError(int retcode) { return ::SSL_get_error(ssl_, retcode); }

int RawChannel::read(void *data, size_t n) { return ::read(fd_, data, n); }

int RawChannel::write(const void *data, size_t n) {
  return ::write(fd_, data, n);
}

void RawChannel::close() {
  ::close(fd_);
  closed_ = true;
}

int RawChannel::getError(int retcode) { return errno; }
