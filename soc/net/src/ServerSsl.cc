#include "../include/ServerSsl.h"

using namespace soc::net;

ServerSsl::ServerSsl(const std::string &certfile, const std::string &pkfile,
                     const std::string &password) {
  init();
  serverCtxCreate();
  serverCertificate(certfile, pkfile, password);
}

ServerSsl::~ServerSsl() {
  if (ctx_)
    ::SSL_CTX_free(ctx_);
}

void ServerSsl::init() {
  ::SSL_library_init();
  ::SSL_load_error_strings();
  ::OpenSSL_add_all_algorithms();
}

void ServerSsl::serverCtxCreate() {
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

void ServerSsl::serverCertificate(const std::string &certfile,
                                  const std::string &pkfile,
                                  const std::string &password) {

  ::SSL_CTX_set_default_passwd_cb_userdata(
      ctx_, password.empty() ? nullptr : (void *)password.c_str());

  if (::SSL_CTX_use_certificate_file(ctx_, certfile.c_str(),
                                     SSL_FILETYPE_PEM) <= 0) {
    ::ERR_print_errors_fp(stderr);
    ::exit(-1);
  }
  if (::SSL_CTX_use_PrivateKey_file(ctx_, pkfile.c_str(), SSL_FILETYPE_PEM) <=
      0) {
    ::ERR_print_errors_fp(stderr);
    ::exit(-1);
  }
  if (::SSL_CTX_check_private_key(ctx_) <= 0) {
    ::ERR_print_errors_fp(stderr);
    ::exit(-1);
  }
}

SslChannel *ServerSsl::createSslChannel(int connfd) {
  SSL *ssl = ::SSL_new(ctx_);
  ::SSL_set_fd(ssl, connfd);
  return new SslChannel(ssl, connfd);
}
