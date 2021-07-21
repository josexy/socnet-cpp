
#ifndef SOC_HTTP_HTTPRESPONSE_H
#define SOC_HTTP_HTTPRESPONSE_H

#include "../../libjson/include/JsonFormatter.h"
#include "../../libjson/include/JsonParser.h"
#include "../../net/include/TcpConnection.h"
#include "HttpRequest.h"

namespace soc {
namespace http {

class HttpRequest;
class HttpResponseBuilder {
public:
  explicit HttpResponseBuilder(HttpRequest *request, net::Buffer *sender);

  HttpResponseBuilder &version(HttpVersion version) {
    version_ = version;
    return *this;
  };

  HttpResponseBuilder &keepAlive(bool on) {
    keepalive_ = on;
    return *this;
  }

  HttpResponseBuilder &code(int code) {
    code_ = code;
    return *this;
  }

  HttpResponseBuilder &header(const HttpHeader &header) {
    header_.add(header);
    return *this;
  }

  HttpResponseBuilder &header(const std::string &key,
                              const std::string &value) {
    header_.add(key, value);
    return *this;
  }
  HttpResponseBuilder &cookie(const std::string &value) {
    header_.add("Set-Cookie", value);
    return *this;
  }
  HttpResponseBuilder &appendBody(const std::string_view &body) {
    tmp_buffer_.append(body.data(), body.size());
    resp_file_ = false;
    return *this;
  }

  HttpResponseBuilder &body(const std::string_view &body) {
    tmp_buffer_.retiredAll();
    appendBody(body);
    return *this;
  }

  HttpResponseBuilder &renderFile(const std::string_view &filename) {
    file_name_ = filename;
    resp_file_ = true;
    return *this;
  }

  HttpResponseBuilder &renderHtml(const std::string_view &filename) {
    file_name_ = filename;
    resp_file_ = true;
    header_.add("Content-Type", "text/html; charset=utf-8");
    return *this;
  }

  HttpResponseBuilder &renderJson(libjson::JsonObject *root,
                                  bool format = true) {
    if (format) {
      libjson::JsonFormatter formatter(4, true);
      formatter.set_source(root);
      body(formatter.format());
    } else {
      body(root->toString());
    }
    header_.add("Content-Type", "application/json; charset=utf-8");
    return *this;
  }

  HttpResponseBuilder &auth(HttpAuthType type);

  HttpVersion version() const noexcept { return version_; }
  int code() const noexcept { return code_; }
  bool keepAlive() const noexcept { return keepalive_; }
  const HttpHeader &header() const noexcept { return header_; }
  std::string_view body() const noexcept {
    return std::string_view(tmp_buffer_.peek(), tmp_buffer_.readable());
  }

  void build();

private:
  void prepareHeader();

private:
  std::string uri_;
  std::string file_name_;

  HttpVersion version_;
  HttpHeader header_;

  bool resp_file_;
  bool keepalive_;
  bool compressed_;
  int code_;

  net::Buffer tmp_buffer_;
  net::Buffer *sender_;
};

// HttpResponse
class HttpResponse {
public:
  friend class HttpServer;

  explicit HttpResponse(net::TcpConnection *conn);

  ~HttpResponse() {
    if (builder_)
      delete builder_;
  }

  HttpVersion version() const noexcept { return builder_->version(); }
  const HttpHeader &header() const noexcept { return builder_->header(); }
  int code() const noexcept { return builder_->code(); }

  HttpResponse &version(HttpVersion version) {
    builder_->version(version);
    return *this;
  }
  HttpResponse &code(int code) {
    builder_->code(code);
    return *this;
  }
  HttpResponse &header(const HttpHeader &header) {
    builder_->header(header);
    return *this;
  }
  HttpResponse &header(const std::string &key, const std::string &value) {
    builder_->header(key, value);
    return *this;
  }
  HttpResponse &cookie(const std::string &value) {
    builder_->cookie(value);
    return *this;
  }
  HttpResponse &cookie(const HttpCookie &cookie) {
    builder_->cookie(cookie.toString());
    return *this;
  }
  HttpResponse &appendBody(const std::string_view &body) {
    builder_->appendBody(body);
    return *this;
  }
  HttpResponse &body(const std::string_view &body) {
    builder_->body(body);
    return *this;
  }
  HttpResponse &bodyJson(libjson::JsonObject *jsonObject, bool format = true) {
    builder_->renderJson(jsonObject, format);
    return *this;
  }
  HttpResponse &bodyHtml(const std::string_view &filename) {
    builder_->renderHtml(filename);
    return *this;
  }
  HttpResponse &bodyFile(const std::string_view &filename) {
    builder_->renderFile(filename);
    return *this;
  }

  void sendAuth(HttpAuthType type = HttpAuthType::Basic);
  void sendRedirect(const std::string &url);

private:
  void send();

private:
  net::TcpConnection *conn_;
  HttpResponseBuilder *builder_;
};
} // namespace http
} // namespace soc
#endif
