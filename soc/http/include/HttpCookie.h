#ifndef SOC_HTTP_HTTPCOOKIE_H
#define SOC_HTTP_HTTPCOOKIE_H

#include "HttpMap.h"

namespace soc {
namespace http {

class   HttpCookie : public HttpMap<std::string, std::string> {
public:
  HttpCookie() : max_age_(0), secure_(false), http_only_(false) {}
  ~HttpCookie() {}

  std::string toString() const;

  void setExpires(const std::string &expires) { expires_ = expires; }
  const std::string &expires() const noexcept { return expires_; }
  void setDomain(const std::string &domain) { domain_ = domain; }
  const std::string &domain() const noexcept { return domain_; }
  void setPath(const std::string &path) { path_ = path; }
  const std::string &path() const noexcept { return path_; }
  void setMaxAge(int second) { max_age_ = second; }
  int maxAge() const noexcept { return max_age_; }
  void setSecure(bool secure) { secure_ = secure; }
  bool secure() const noexcept { return secure_; }
  void setHttpOnly(bool httponly) { http_only_ = httponly; }
  bool httpOnly() const noexcept { return http_only_; }

private:
  std::string expires_;
  std::string domain_;
  std::string path_;
  int max_age_;
  bool secure_;
  bool http_only_;
};

} // namespace http
} // namespace soc

#endif
