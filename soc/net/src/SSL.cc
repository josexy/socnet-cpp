#include "../include/SSL.h"

using namespace soc::net;

SSLTls::SSLTls(const char *server_cert_file, const char *server_private_file,
               const char *password) {
  ssl_init();
  ssl_server_ctx_create();
  ssl_server_certificate(server_cert_file, server_private_file, password);
}

SSLTls::~SSLTls() {
  if (ctx_)
    ::SSL_CTX_free(ctx_);
}

void SSLTls::ssl_init() {
  ::SSL_library_init();
  ::SSL_load_error_strings();
  ::OpenSSL_add_all_algorithms();
}

void SSLTls::ssl_server_ctx_create() {
  ctx_ = ::SSL_CTX_new(TLS_server_method());
  if (!ctx_) {
    ::ERR_print_errors_fp(stderr);
    ::exit(-1);
  }
  ::SSL_CTX_set_options(ctx_,
                        SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                            SSL_OP_NO_COMPRESSION |
                            SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
}

void SSLTls::ssl_server_certificate(const char *cert_file,
                                    const char *private_key_file,
                                    const char *password) {
  ::SSL_CTX_set_default_passwd_cb_userdata(ctx_, (void *)password);

  if (::SSL_CTX_use_certificate_file(ctx_, cert_file, SSL_FILETYPE_PEM) <= 0) {
    ::ERR_print_errors_fp(stderr);
    ::exit(-1);
  }
  if (::SSL_CTX_use_PrivateKey_file(ctx_, private_key_file, SSL_FILETYPE_PEM) <=
      0) {
    ::ERR_print_errors_fp(stderr);
    ::exit(-1);
  }
  if (::SSL_CTX_check_private_key(ctx_) <= 0) {
    ::ERR_print_errors_fp(stderr);
    ::exit(-1);
  }
}

SSL *SSLTls::ssl_create_conn(int connfd) {
  SSL *ssl = ::SSL_new(ctx_);
  ::SSL_set_fd(ssl, connfd);
  return ssl;
}
