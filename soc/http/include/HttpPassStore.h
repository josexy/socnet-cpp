#ifndef SOC_HTTP_HTTPPASSSTORE_H
#define SOC_HTTP_HTTPPASSSTORE_H

#include "../../utility/include/EncodeUtil.h"
#include "../../utility/include/FileUtil.h"
#include "HttpMap.h"

namespace soc {
namespace http {

static constexpr const char *kPassFile = "./pass_store/user_password";
static constexpr const char *kRealm = "socnet@test";

class HttpPassStore {
public:
  static HttpPassStore &instance() {
    static HttpPassStore ps;
    return ps;
  }

  std::optional<std::string> fetch(const std::string &user) const {
    return user_pass_.get(user);
  }

private:
  HttpPassStore(const HttpPassStore &) = delete;
  HttpPassStore(HttpPassStore &&) = delete;
  HttpPassStore &operator=(const HttpPassStore &) = delete;
  HttpPassStore &operator=(HttpPassStore &&) = delete;

  HttpPassStore() {
    FileUtil loader(kPassFile);
    if (!loader.isOpen()) {
      ::fprintf(stderr, "user-password file not found : %s\n", kPassFile);
      ::exit(-1);
    }
    std::string line;
    while (loader.readLine(line)) {
      // {user:md5(username:realm:password)}
      size_t pos = line.find_first_of(":");
      user_pass_.add(line.substr(0, pos), line.substr(pos + 1));
    }
  }

  HttpMap<std::string, std::string> user_pass_;
};

} // namespace http
} // namespace soc

#endif
