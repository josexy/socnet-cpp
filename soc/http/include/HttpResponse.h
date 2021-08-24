
#ifndef SOC_HTTP_HTTPRESPONSE_H
#define SOC_HTTP_HTTPRESPONSE_H

#include "../../libjson/include/JsonFormatter.h"
#include "../../libjson/include/JsonParser.h"
#include "HttpRequest.h"

namespace soc {
namespace http {

class HttpRequest;
class HttpResponseBuilder {
public:
  explicit HttpResponseBuilder(HttpRequest *request, net::TcpConnection *conn);

  HttpResponseBuilder &setVersion(HttpVersion version) {
    version_ = version;
    return *this;
  };

  HttpResponseBuilder &setKeepAlive(bool on) {
    keepalive_ = on;
    return *this;
  }

  HttpResponseBuilder &setCode(int code) {
    code_ = code;
    return *this;
  }

  HttpResponseBuilder &setHeader(const HttpHeader &header) {
    header_.add(header);
    return *this;
  }

  HttpResponseBuilder &setHeader(const std::string &key,
                                 const std::string &value) {
    header_.add(key, value);
    return *this;
  }
  HttpResponseBuilder &setCookie(const std::string &value) {
    header_.add("Set-Cookie", value);
    return *this;
  }
  HttpResponseBuilder &appendBody(const std::string_view &body) {
    tmp_buffer_.append(body.data(), body.size());
    resp_file_ = false;
    return *this;
  }

  HttpResponseBuilder &setBody(const std::string_view &body) {
    tmp_buffer_.reset();
    appendBody(body);
    return *this;
  }

  HttpResponseBuilder &setBodyFile(const std::string_view &filename) {
    tmp_buffer_.reset();
    file_name_ = filename;
    resp_file_ = true;
    return *this;
  }

  HttpResponseBuilder &setBodyHtml(const std::string_view &filename) {
    tmp_buffer_.reset();
    file_name_ = filename;
    resp_file_ = true;
    header_.add("Content-Type", "text/html; charset=utf-8");
    return *this;
  }

  HttpResponseBuilder &setBodyJson(libjson::JsonObject *root,
                                   bool format = true) {
    if (format) {
      libjson::JsonFormatter formatter(4, true);
      formatter.set_source(root);
      setBody(formatter.format());
    } else {
      setBody(root->toString());
    }
    header_.add("Content-Type", "application/json; charset=utf-8");
    return *this;
  }

  HttpResponseBuilder &setAuthType(HttpAuthType type);

  HttpVersion getVersion() const noexcept { return version_; }
  int getCode() const noexcept { return code_; }
  bool isKeepAlive() const noexcept { return keepalive_; }
  const HttpHeader &getHeader() const noexcept { return header_; }
  std::string_view getBody() noexcept {
    return std::string_view(tmp_buffer_.peek(), tmp_buffer_.readable());
  }

  void build();
  net::TcpConnection *connection() const { return conn_; }

private:
  void prepareHeader();
  void makeHeaderPart(size_t);

private:
  std::string uri_;
  std::string file_name_;

  HttpVersion version_;
  HttpHeader header_;
  HttpMethod method_;

  bool resp_file_;
  bool keepalive_;
  bool compressed_;
  int code_;

  net::Buffer tmp_buffer_;
  net::TcpConnection *conn_;
};

// HttpResponse
class HttpResponse {
public:
  friend class HttpServer;

  explicit HttpResponse(net::TcpConnection *conn);

  ~HttpResponse() {
    if (builder_)
      delete builder_;
    builder_ = nullptr;
  }

  HttpVersion getVersion() const noexcept { return builder_->getVersion(); }
  const HttpHeader &getHeader() const noexcept { return builder_->getHeader(); }
  int getCode() const noexcept { return builder_->getCode(); }

  HttpResponse &setContentType(const std::string &content_type) {
    return setHeader("Content-Type", content_type);
  }
  HttpResponse &setVersion(HttpVersion version) {
    builder_->setVersion(version);
    return *this;
  }
  HttpResponse &setCode(int code) {
    builder_->setCode(code);
    return *this;
  }
  HttpResponse &setHeader(const HttpHeader &header) {
    builder_->setHeader(header);
    return *this;
  }
  HttpResponse &setHeader(const std::string &key, const std::string &value) {
    builder_->setHeader(key, value);
    return *this;
  }
  HttpResponse &setCookie(const std::string &value) {
    builder_->setCookie(value);
    return *this;
  }
  HttpResponse &setCookie(const HttpCookie &cookie) {
    builder_->setCookie(cookie.toString());
    return *this;
  }
  HttpResponse &appendBody(const std::string_view &body) {
    builder_->appendBody(body);
    return *this;
  }
  HttpResponse &setBody(const std::string_view &body) {
    builder_->setBody(body);
    return *this;
  }
  HttpResponse &setBodyJson(libjson::JsonObject *jsonObject,
                            bool format = true) {
    builder_->setBodyJson(jsonObject, format);
    return *this;
  }
  HttpResponse &setBodyHtml(const std::string_view &filename) {
    builder_->setBodyHtml(filename);
    return *this;
  }
  HttpResponse &setBodyFile(const std::string_view &filename) {
    builder_->setBodyFile(filename);
    return *this;
  }

  void sendAuth(HttpAuthType type = HttpAuthType::Basic);
  void sendRedirect(const std::string &url);

private:
  void send();

private:
  HttpResponseBuilder *builder_;
};
} // namespace http
} // namespace soc
#endif
