#ifndef SOC_HTTP_HTTPPASSSTORE_H
#define SOC_HTTP_HTTPPASSSTORE_H

#include "../../utility/include/AppConfig.h"
#include "../../utility/include/EncodeUtil.h"
#include "../../utility/include/FileUtil.h"
#include "HttpMap.h"

namespace soc {
namespace http {

class HttpPassStore {
public:
  static HttpPassStore &instance() {
    static HttpPassStore ps;
    return ps;
  }

  std::optional<std::string> fetch(const std::string &user) const {
    return user_pass_.get(user);
  }

  const std::string &getRealm() const noexcept { return realm_; }

private:
  HttpPassStore(const HttpPassStore &) = delete;
  HttpPassStore(HttpPassStore &&) = delete;
  HttpPassStore &operator=(const HttpPassStore &) = delete;
  HttpPassStore &operator=(HttpPassStore &&) = delete;

  HttpPassStore() {
    realm_ = GET_CONFIG(std::string, "server", "authenticate_realm");
    std::string file = GET_CONFIG(std::string, "server", "user_pass_file");

    FileUtil loader(file);
    if (!loader.isOpen()) {
      ::fprintf(stderr, "user-password file not found : [%s]\n", file.c_str());
    } else {
      std::string line;
      while (loader.readLine(line)) {
        // {user:md5(username:realm:password)}
        size_t pos = line.find_first_of(":");
        user_pass_.add(line.substr(0, pos), line.substr(pos + 1));
      }
    }
  }

  HttpMap<std::string, std::string> user_pass_;
  std::string realm_;
};

} // namespace http
} // namespace soc

#endif
