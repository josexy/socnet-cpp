#ifndef SOC_UTILITY_APPCONFIG_H
#define SOC_UTILITY_APPCONFIG_H

#include "../../libjson/include/JsonFormatter.h"
#include "../../libjson/include/JsonParser.h"
#include <unordered_map>
#include <variant>

using namespace libjson;

namespace soc {
class AppConfig {
public:
  using Values = std::variant<std::string, int, bool, std::vector<std::string>>;
  using ConfigMp =
      std::unordered_map<std::string, std::unordered_map<std::string, Values>>;

  static AppConfig &instance() {
    static AppConfig config;
    return config;
  }

  const Values &get(const std::string &key, const std::string &subKey) const;
  bool exist(const std::string &key, const std::string &subKey);

  ~AppConfig() {}

private:
  AppConfig(const std::string &config = "./config.json");

  AppConfig(const AppConfig &) = delete;
  AppConfig &operator==(const AppConfig &) = delete;
  AppConfig(AppConfig &&) = delete;
  AppConfig &operator==(AppConfig &&) = delete;

private:
  ConfigMp config_;
};

#define EXIST_CONFIG(KEY, SUBKEY) AppConfig::instance().exist((KEY), (SUBKEY))

#define GET_CONFIG(T, KEY, SUBKEY)                                             \
  std::get<T>(AppConfig::instance().get((KEY), (SUBKEY)))

} // namespace soc

#endif