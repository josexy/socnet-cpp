#ifndef SOC_HTTP_HTTPAUTH_H
#define SOC_HTTP_HTTPAUTH_H

#include <string.h>

#include "HttpPassStore.h"

namespace soc {
namespace http {

template <class HttpDerivedClass> class HttpAuth {
public:
  const HttpDerivedClass *get_pointer() const {
    return static_cast<const HttpDerivedClass *>(this);
  }

  bool verify_user() const { return get_pointer()->verify_user(); }
};

template <class PassStore>
class HttpBasicAuth : public HttpAuth<HttpBasicAuth<PassStore>> {
public:
  friend class HttpRequest;

  bool verify_user() const noexcept { return verify_successed_; }

private:
  void verify_user_private(const std::string_view &user,
                           const std::string_view &pass) {
    auto x = HttpPassStore<PassStore>::instance().fetch_password(user);
    if (!x.first)
      return;
    std::string hashv =
        std::string(user) + ":" + kRealm + ":" + std::string(pass);
    verify_successed_ = EncodeUtil::md5_hash_equal(hashv, x.second);
  }

  bool verify_successed_ = false;
};

template <class PassStore>
class HttpDigestAuth : public HttpAuth<HttpDigestAuth<PassStore>> {
public:
  friend class HttpRequest;

  bool verify_user() const noexcept { return verify_successed_; }

private:
  void verify_user_private(
      const std::string_view &user, const std::string_view &method_,
      const std::string_view &uri, const std::string_view &nonce,
      const std::string_view &nc, const std::string_view &cnonce,
      const std::string_view &qop, const std::string_view &resp) {
    // h1 = md5(username:realm:password)
    // h2 = md5(method:uri)
    // resp = md5(h1:nonce:nc:cnonce:qop:h2)

    // clear password
    auto x = HttpPassStore<PassStore>::instance().fetch_password(user);
    if (!x.first)
      return;

    char buffer[128]{0};
    // h1
    std::string h1 = x.second;

    // h2
    ::memset(buffer, 0, 128);
    ::sprintf(buffer, "%s:%s", method_.data(), uri.data());
    std::string h2 = EncodeUtil::md5_hash(buffer);

    // resp
    ::memset(buffer, 0, 128);
    ::strncat(buffer, h1.data(), h1.size());
    if (nonce.size()) {
      ::strncat(buffer, ":", 2);
      ::strncat(buffer, nonce.data(), nonce.size());
    }
    if (nc.size()) {
      ::strncat(buffer, ":", 2);
      ::strncat(buffer, nc.data(), nc.size());
    }
    if (cnonce.size()) {
      ::strncat(buffer, ":", 2);
      ::strncat(buffer, cnonce.data(), cnonce.size());
    }
    if (qop.size()) {
      ::strncat(buffer, ":", 2);
      ::strncat(buffer, qop.data(), qop.size());
    }
    ::strncat(buffer, ":", 2);
    ::strncat(buffer, h2.data(), h2.size());

    verify_successed_ = EncodeUtil::md5_hash_equal(buffer, resp.data());
  }

private:
  bool verify_successed_ = false;
};

} // namespace http
} // namespace soc

#endif
