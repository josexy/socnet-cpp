#ifndef SOC_MODULE_MYSQL_H
#define SOC_MODULE_MYSQL_H

#include "../../../utility/include/AppConfig.h"
#include <assert.h>
#include <mysql/mysql.h>

namespace soc {
class MYSQLManager {
public:
  static MYSQLManager &instance() noexcept {
    static MYSQLManager manager(GET_CONFIG(std::string, "mysql", "host"),
                                GET_CONFIG(std::string, "mysql", "username"),
                                GET_CONFIG(std::string, "mysql", "password"),
                                GET_CONFIG(int, "mysql", "port"));
    return manager;
  }

  explicit MYSQLManager(const std::string_view &host,
                        const std::string_view &user,
                        const std::string_view &password, uint16_t port)
      : open_(false) {
    ::mysql_init(&mysql_);
    assert(connect(host, user, password, port));
  }
  ~MYSQLManager() { close(); }

  bool connected() const noexcept { return open_; }

  bool connect(const std::string_view &host, const std::string_view &user,
               const std::string_view &password, uint16_t port) {
    if (open_)
      return false;
    MYSQL *m = ::mysql_real_connect(&mysql_, host.data(), user.data(),
                                    password.data(), nullptr, port, nullptr, 0);
    if (!m)
      return false;
    ::mysql_set_character_set(&mysql_, "utf8");
    return open_ = true;
  }

  bool execute(const std::string_view &query) {
    return 0 == ::mysql_real_query(&mysql_, query.data(), query.size());
  }

  void get_result(std::vector<std::string> &res) {
    res_ = ::mysql_store_result(&mysql_);
    if (!res_)
      return;
    MYSQL_ROW row;
    size_t cols = ::mysql_num_fields(res_);
    // Only fetch one row
    if ((row = ::mysql_fetch_row(res_))) {
      for (size_t i = 0; i < cols; ++i) {
        res.emplace_back(row[i]);
      }
    }
    ::mysql_free_result(res_);
  }

  void close() {
    if (!open_)
      return;
    ::mysql_close(&mysql_);
    open_ = false;
  }

private:
  MYSQL mysql_;
  MYSQL_RES *res_;
  bool open_;
};

} // namespace soc

#endif
