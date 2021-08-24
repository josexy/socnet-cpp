#ifndef SOC_NET_SERVERSSL_H
#define SOC_NET_SERVERSSL_H

#include "Channel.h"

namespace soc {
namespace net {

class ServerSsl {
public:
  explicit ServerSsl(const std::string &certfile, const std::string &pkfile,
                     const std::string &password);
  ~ServerSsl();

  SSL_CTX *getSslCtx() const noexcept { return ctx_; }
  SslChannel *createSslChannel(int connfd);
  int accept(SslChannel *channel) { return ::SSL_accept(channel->ssl()); }

private:
  void init();
  void serverCtxCreate();
  void serverCertificate(const std::string &, const std::string &,
                         const std::string &);

private:
  SSL_CTX *ctx_;
};
} // namespace net
} // namespace soc

#endif
