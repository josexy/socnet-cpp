#ifndef SOC_NET_CHANNEL_H
#define SOC_NET_CHANNEL_H

#include "Buffer.h"
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <unistd.h>

namespace soc {
namespace net {

enum ChannelType { Raw, Ssl };

class Channel {
public:
  virtual ~Channel() {}
  virtual int read(void *data, size_t n) = 0;
  virtual int write(const void *data, size_t n) = 0;
  virtual void close() = 0;
  virtual int getError(int) = 0;
  virtual ChannelType type() = 0;
};

class RawChannel : public Channel {
public:
  explicit RawChannel(int fd) : fd_(fd), closed_(false) {}
  virtual ~RawChannel() {
    if (!closed_)
      close();
  }

  int read(void *data, size_t n) override;
  int write(const void *data, size_t n) override;
  void close() override;
  int getError(int) override;
  ChannelType type() override { return ChannelType::Raw; }

private:
  int fd_;
  bool closed_;
};

class SslChannel : public RawChannel {
public:
  explicit SslChannel(SSL *ssl, int fd) : RawChannel(fd), ssl_(ssl) {}
  ~SslChannel() {
    if (ssl_)
      this->close();
  }

  int read(void *data, size_t n) override;
  int write(const void *data, size_t n) override;
  void close() override;
  int getError(int) override;
  ChannelType type() override { return ChannelType::Ssl; }

  SSL *ssl() const noexcept { return ssl_; }

private:
  SSL *ssl_;
};
} // namespace net
} // namespace soc

#endif