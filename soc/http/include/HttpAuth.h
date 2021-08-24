#ifndef SOC_HTTP_HTTPAUTH_H
#define SOC_HTTP_HTTPAUTH_H

#include "HttpPassStore.h"

namespace soc {
namespace http {

enum class HttpAuthType { Basic = 0x01, Digest = 0x02 };

class HttpAuth {
public:
  virtual bool verify() const = 0;
  virtual HttpAuthType getAuthType() const noexcept = 0;
};

class HttpBasicAuth : public HttpAuth {
public:
  explicit HttpBasicAuth(std::string_view user, std::string_view pass) {
    user_ = std::string(user.data(), user.size());
    pass_ = std::string(pass.data(), pass.size());
    auto x = HttpPassStore::instance().fetch(user_);
    // user authentication succeeded
    if (x.has_value()) {
      std::string hashv =
          user_ + ":" + HttpPassStore::instance().getRealm() + ":" + pass_;
      verify_successed_ = EncodeUtil::md5HashEqual(hashv, x.value());
    }
  }

  const std::string &getUsername() const noexcept { return user_; }
  const std::string &getPassword() const noexcept { return pass_; }
  bool verify() const override { return verify_successed_; }
  HttpAuthType getAuthType() const noexcept { return HttpAuthType::Basic; }

private:
  bool verify_successed_ = false;
  std::string user_;
  std::string pass_;
};

class HttpDigestAuth : public HttpAuth {
public:
  explicit HttpDigestAuth(const std::string &digest_info,
                          const HttpMap<std::string, std::string> &da)
      : digest_info_(digest_info) {
    // h1 = md5(username:realm:password)
    // h2 = md5(method:uri)
    // resp = md5(h1:nonce:nc:cnonce:qop:h2)

    auto username = da.get("username");
    if (!username.has_value())
      return;
    // clear password
    auto x = HttpPassStore::instance().fetch(username.value());
    if (!x.has_value())
      return;

    auto method = da.get("method");
    auto uri = da.get("uri");
    auto nonce = da.get("nonce");
    auto nc = da.get("nc");
    auto cnonce = da.get("cnonce");
    auto qop = da.get("qop");
    auto response = da.get("response");

    char buffer[128]{0};
    // h1
    std::string h1 = x.value();

    // h2
    ::memset(buffer, 0, 128);
    ::sprintf(buffer, "%s:%s", method.value().data(), uri.value().data());
    std::string h2 = EncodeUtil::md5Hash(buffer);

    // resp
    ::memset(buffer, 0, 128);
    ::strncat(buffer, h1.data(), h1.size());
    if (nonce.has_value()) {
      ::strncat(buffer, ":", 2);
      ::strcat(buffer, nonce.value().data());
    }
    if (nc.has_value()) {
      ::strncat(buffer, ":", 2);
      ::strcat(buffer, nc.value().data());
    }
    if (cnonce.has_value()) {
      ::strncat(buffer, ":", 2);
      ::strcat(buffer, cnonce.value().data());
    }
    if (qop.has_value()) {
      ::strncat(buffer, ":", 2);
      ::strcat(buffer, qop.value().data());
    }
    ::strncat(buffer, ":", 2);
    ::strncat(buffer, h2.data(), h2.size());

    verify_successed_ =
        EncodeUtil::md5HashEqual(buffer, response.value().data());
  }

  const std::string &getDigestInfo() const noexcept { return digest_info_; }
  bool verify() const override { return verify_successed_; }
  HttpAuthType getAuthType() const noexcept { return HttpAuthType::Digest; }

private:
  bool verify_successed_ = false;
  std::string digest_info_;
};

} // namespace http
} // namespace soc

#endif
