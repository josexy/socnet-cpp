
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

  HttpResponseBuilder &close(bool on) {
    close_ = on;
    return *this;
  }

  HttpResponseBuilder &status_code(int code) {
    status_code_ = code;
    return *this;
  }

  HttpResponseBuilder &header(const HttpHeader &header) {
    header_ = std::move(header);
    return *this;
  }

  HttpResponseBuilder &header(const std::string &key,
                              const std::string &value) {
    header_.add(key, value);
    return *this;
  }

  HttpResponseBuilder &append_body(const std::string_view &body) {
    body_.append(body.data(), body.size());
    return *this;
  }

  HttpResponseBuilder &body(const std::string_view &body) {
    body_.assign(body.data(), body.size());
    return *this;
  }

  HttpResponseBuilder &render_file(const std::string_view &filename) {
    FileLoader(filename).read_all(body_);
    return *this;
  }

  HttpResponseBuilder &render_html(const std::string_view &filename) {
    FileLoader(filename).read_all(body_);
    header_.add("Content-Type", "text/html; charset=utf-8");
    return *this;
  }

  HttpResponseBuilder &render_json(libjson::JsonObject *root,
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

  HttpResponseBuilder &auth(HttpAuthType auth_type = HttpAuthType::Basic);

  HttpVersion version() const noexcept { return version_; }
  int status_code() const noexcept { return status_code_; }
  const HttpHeader &header() const noexcept { return header_; }
  const std::string &body() const noexcept { return body_; }
  bool close() const noexcept { return close_; }

  void build();

private:
  void prepare_header();

private:
  std::string uri_;
  std::string body_;

  HttpVersion version_;
  HttpHeader header_;

  bool close_;
  bool compressed_;
  int status_code_;

  net::Buffer *sender_;
};

// HttpResponse
class HttpResponse {
public:
  explicit HttpResponse(net::TcpConnectionPtr conn);

  ~HttpResponse() {
    if (builder_)
      delete builder_;
  }

  // Getter
  HttpVersion version() const noexcept { return builder_->version(); }
  const HttpHeader &header() const noexcept { return builder_->header(); }
  int status_code() const noexcept { return builder_->status_code(); }

  // Setter
  HttpResponse &version(HttpVersion version) {
    builder_->version(version);
    return *this;
  }
  HttpResponse &status_code(int code) {
    builder_->status_code(code);
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
  HttpResponse &auth(HttpAuthType auth_type = HttpAuthType::Basic) {
    builder_->auth(auth_type);
    return *this;
  }
  HttpResponse &append_body(const std::string_view &body) {
    builder_->append_body(body);
    return *this;
  }
  HttpResponse &body(const std::string_view &body) {
    builder_->body(body);
    return *this;
  }
  HttpResponse &body_json(libjson::JsonObject *jsonObject, bool format = true) {
    builder_->render_json(jsonObject, format);
    return *this;
  }
  HttpResponse &body_html(const std::string_view &filename) {
    builder_->render_html(filename);
    return *this;
  }
  HttpResponse &body_file(const std::string_view &filename) {
    builder_->render_file(filename);
    return *this;
  }

  void send();

private:
  net::TcpConnectionPtr conn_;
  HttpResponseBuilder *builder_;
};
} // namespace http
} // namespace soc
#endif
