#include "../include/AppConfig.h"
using namespace soc;

AppConfig::AppConfig(const std::string &config) {
  std::ifstream ifs(config, std::ios_base::in);
  JsonParser parser(ifs);
  ifs.close();
  auto [objPtr, arrPtr, valuePtr] = parser.parse();

  size_t n = objPtr->size();
  for (size_t i = 0; i < n; i++) {
    auto obj = objPtr->get(i);
    auto value = obj.second->toJsonObject();
    std::string key = obj.first->value<std::string>();
    size_t m = value->size();
    for (size_t j = 0; j < m; j++) {
      auto subobj = value->get(j);
      auto subvalue = subobj.second;
      std::string subkey = subobj.first->value<std::string>();

      switch (subvalue->type()) {
      case JsonType::String:
        config_[key][subkey] = subvalue->value<std::string>();
        break;
      case JsonType::Number:
        config_[key][subkey] = subvalue->value<int>();
        break;
      case JsonType::Boolean:
        config_[key][subkey] = subvalue->value<bool>();
        break;
      case JsonType::Array: {
        auto arr = subvalue->toJsonArray();
        std::vector<std::string> values;
        for (size_t k = 0; k < arr->size(); k++) {
          auto val = arr->get(k);
          values.emplace_back(val->value<std::string>());
        }
        config_[key][subkey] = values;
      } break;
      default:
        break;
      }
    }
  }
}

const AppConfig::Values &AppConfig::get(const std::string &key,
                                        const std::string &subKey) const {
  return config_.at(key).at(subKey);
}

bool AppConfig::exist(const std::string &key, const std::string &subKey) {
  return config_.find(key) != config_.end() &&
         config_[key].find(subKey) != config_[key].end();
}
