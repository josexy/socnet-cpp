#ifndef SOC_HTTP_HTTPREQUEST_H
#define SOC_HTTP_HTTPREQUEST_H

#include "HttpAuth.h"
#include "HttpHeader.h"
#include "HttpMultiPart.h"
#include "HttpUtil.h"
#include <variant>

namespace soc {
namespace http {

using HttpBasicLocalAuth = HttpBasicAuth<HttpLocalPassStore>;
using HttpBasicSqlAuth = HttpBasicAuth<HttpSqlPassStore>;
using HttpDigestLocalAuth = HttpDigestAuth<HttpLocalPassStore>;
using HttpDigestSqlAuth = HttpDigestAuth<HttpSqlPassStore>;

class HttpRequest {
public:
  enum RetCode {
    NO_REQUEST,

    REQUEST_LINE_DONE,
    REQUEST_HEADER_DONE,
    REQUEST_CONTENT_DONE,

    BAD_REQUEST,

    AGAIN_HEADER,
    AGAIN_CONTENT
  };

  explicit HttpRequest(net::Buffer *recver);
  void initialize_parse();

  void add(const std::string &key, const std::string &value);

  std::optional<std::string> get(const std::string &key) const;
  HttpMethod method() const noexcept { return method_; }
  HttpVersion version() const noexcept { return version_; }
  HttpAuthType auth_type() const noexcept { return auth_type_; }
  const HttpHeader &header() const noexcept { return header_; }
  const std::string &query_s() const noexcept { return query_s_; }
  const HttpMultiPart &multipart() const noexcept { return multipart_; }
  const std::string &url() const noexcept { return url_; }
  std::string full_url() const noexcept;

  bool has_query() const noexcept { return !query_.empty(); }
  bool has_form() const noexcept { return !form_.empty(); }
  bool close_conn() const noexcept { return close_; }
  bool compressed() const noexcept { return compressed_; }
  bool has_multipart() const noexcept { return has_multipart_; }
  bool has_cookies() const noexcept { return has_cookies_; }

  decltype(auto) query() const noexcept { return query_; }
  decltype(auto) form() const noexcept { return form_; }
  decltype(auto) cookies() const noexcept { return cookies_; }
  decltype(auto) post_data() const noexcept { return post_data_; }

  template <class T> decltype(auto) auth() const {
    return const_cast<T *>(std::get_if<T>(&auth_));
  }

  RetCode parse_request();

private:
  inline constexpr static char CR = '\r';
  inline constexpr static char LF = '\n';

  RetCode parse_request_line();
  RetCode parse_request_header();
  RetCode parse_request_content();

  void parse_url_query();
  void parse_authorization();

  void parse_key_value(std::string_view, const std::string_view &,
                       const std::string_view &,
                       std::unordered_map<std::string, std::string> &);

private:
  net::Buffer *recver_;

  std::string req_url_;
  std::string url_;
  std::string query_s_;
  std::string post_data_;
  HttpMethod method_;
  HttpVersion version_;
  HttpHeader header_;
  HttpMultiPart multipart_;
  HttpAuthType auth_type_;
  RetCode ret_code_;

  bool close_;
  bool compressed_;
  bool has_multipart_;
  bool has_cookies_;

  std::variant<HttpBasicLocalAuth, HttpBasicSqlAuth, HttpDigestLocalAuth,
               HttpDigestSqlAuth>
      auth_;

  std::unordered_map<std::string, std::string> query_;
  std::unordered_map<std::string, std::string> form_;
  std::unordered_map<std::string, std::string> cookies_;
  std::unordered_map<std::string, std::string> da_;
};
} // namespace http
} // namespace soc
#endif
