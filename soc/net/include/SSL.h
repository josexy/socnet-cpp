#ifndef SOC_NET_SSLTLS_H
#define SOC_NET_SSLTLS_H

#include <openssl/err.h>
#include <openssl/ssl.h>

namespace soc {
namespace net {
class SSLTls {
public:
  explicit SSLTls(const char *server_cert_file, const char *server_private_file,
                  const char *password = nullptr);
  ~SSLTls();
  int accept_conn(SSL *ssl) { return ::SSL_accept(ssl); }
  SSL_CTX *ctx() const noexcept { return ctx_; }
  SSL *ssl_create_conn(int connfd);

private:
  void ssl_init();
  void ssl_server_ctx_create();
  void ssl_server_certificate(const char *cert_file,
                              const char *private_key_file,
                              const char *password = nullptr);

private:
  SSL_CTX *ctx_;
};
} // namespace net
} // namespace soc

#endif
