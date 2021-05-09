#ifndef SOC_HTTP_HTTPPASSSTORE_H
#define SOC_HTTP_HTTPPASSSTORE_H

#include "../../modules/mysql/include/MYSQL.h"
#include "../../utility/include/EncodeUtil.h"
#include "../../utility/include/FileLoader.h"
#include "../../utility/include/MacroCtl.h"
namespace soc {
namespace http {

template <class DerivedStore> class HttpPassStore {
public:
  static DerivedStore &instance() {
    static DerivedStore ps;
    return ps;
  }

private:
  HttpPassStore(const HttpPassStore &) = delete;
  HttpPassStore(HttpPassStore &&) = delete;
  HttpPassStore &operator=(const HttpPassStore &) = delete;
  HttpPassStore &operator=(HttpPassStore &&) = delete;

  HttpPassStore() {}
  friend DerivedStore;
};

class HttpLocalPassStore : public HttpPassStore<HttpLocalPassStore> {
public:
  template <class> friend class soc::http::HttpPassStore;

  std::pair<bool, std::string>
  fetch_password(const std::string_view &user) const {
    std::string x(user.data(), user.size());
    if (user_pass_tb_.find(x) == user_pass_tb_.end())
      return std::make_pair(false, "");
    return std::make_pair(true, user_pass_tb_.at(x));
  }

private:
  HttpLocalPassStore() {
    FileLoader loader(kPassFile);
    assert(loader.is_open());
    std::string line;
    while (loader.read_line(line)) {
      // {user:md5(username:realm:password)}
      size_t pos = line.find_first_of(":");
      user_pass_tb_.emplace(line.substr(0, pos), line.substr(pos + 1));
    }
  }

  std::unordered_map<std::string, std::string> user_pass_tb_;
};

#ifdef SUPPORT_MYSQL_LIB

class HttpSqlPassStore : public HttpPassStore<HttpSqlPassStore> {
public:
  template <class> friend class soc::http::HttpPassStore;

  std::pair<bool, std::string>
  fetch_password(const std::string_view &user) const {
    std::string query = "select `password` from " +
                        Env::instance().get("MYSQL_DB").value() + "." +
                        Env::instance().get("MYSQL_TB").value() +
                        " where `user`= '" + std::string(user) + "'";

    if (!MYSQLManager::instance().execute(query)) {
      return std::make_pair(false, "");
    }
    std::vector<std::string> row;
    MYSQLManager::instance().get_result(row);
    if (row.empty())
      return std::make_pair(false, "");

    std::string hash_password = row[0];
    return std::make_pair(true, hash_password);
  }
};
#endif
} // namespace http
} // namespace soc

#endif
