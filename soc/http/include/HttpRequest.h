#ifndef SOC_HTTP_HTTPREQUEST_H
#define SOC_HTTP_HTTPREQUEST_H

#include "HttpAuth.h"
#include "HttpCookie.h"
#include "HttpMultiPart.h"
#include "HttpSession.h"
#include "HttpSessionServer.h"

namespace soc {
namespace http {

class HttpRequest {
public:
  friend class HttpService;
  friend class HttpServer;

  enum RetCode {
    NO_REQUEST,

    REQUEST_LINE_DONE,
    REQUEST_HEADER_DONE,
    REQUEST_CONTENT_DONE,

    BAD_REQUEST,

    AGAIN_HEADER,
    AGAIN_CONTENT
  };

  explicit HttpRequest(net::Buffer *recver, HttpSessionServer *owner);
  ~HttpRequest();

  void initialize();
  void add(const std::string &key, const std::string &value);
  std::optional<std::string> get(const std::string &key) const;
  HttpMethod method() const noexcept { return method_; }
  HttpVersion version() const noexcept { return version_; }
  const HttpMultiPart &multiPart() const noexcept { return multipart_; }
  const std::string &queryStr() const noexcept { return query_s_; }
  const std::string &url() const noexcept { return url_; }
  const std::string &phpMessage() const noexcept { return php_message_; }
  std::string fullUrl() const noexcept;

  bool hasQueryString() const noexcept { return !query_.empty(); }
  bool hasForm() const noexcept { return !form_.empty(); }
  bool hasMultiPart() const noexcept { return has_multipart_; }
  bool hasCookies() const noexcept { return has_cookies_; }
  bool isKeepAlive() const noexcept { return keepalive_; }
  bool isCompressed() const noexcept { return compressed_; }

  const HttpHeader &header() const noexcept { return header_; }
  const HttpMap<std::string, std::string> &query() const noexcept {
    return query_;
  }
  const HttpMap<std::string, std::string> &form() const noexcept {
    return form_;
  }
  const HttpCookie &cookies() const noexcept { return cookies_; }
  const std::string &postData() const noexcept { return post_data_; }
  const std::vector<std::string> &match() const noexcept { return match_; }

  HttpAuth *auth() const noexcept { return auth_; }
  HttpSession *session() const;

  RetCode parseRequest();

private:
  inline constexpr static char CR = '\r';
  inline constexpr static char LF = '\n';

  RetCode parseRequestLine();
  RetCode parseRequestHeader();
  RetCode parseRequestContent();

  void parseUrlQuery();
  void parseCookie();
  void parseMultiPartData();
  void parseAuthorization();

  void parseKeyValue(std::string_view, const std::string_view &,
                     const std::string_view &,
                     HttpMap<std::string, std::string> &);

private:
  net::Buffer *recver_;
  HttpSessionServer *owner_;

  std::string req_url_;
  std::string url_;
  std::string query_s_;
  std::string post_data_;
  HttpMethod method_;
  HttpVersion version_;
  HttpHeader header_;
  HttpMultiPart multipart_;
  RetCode ret_code_;
  HttpCookie cookies_;

  bool keepalive_;
  bool compressed_;
  bool has_multipart_;
  bool has_cookies_;

  HttpAuth *auth_;
  mutable HttpSession *session_;

  mutable std::vector<std::string> match_;
  mutable std::string php_message_;

  HttpMap<std::string, std::string> query_;
  HttpMap<std::string, std::string> form_;
  HttpMap<std::string, std::string> da_;
};
} // namespace http
} // namespace soc
#endif
