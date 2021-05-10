#ifndef SOC_NET_ENV_H
#define SOC_NET_ENV_H

#include <stdlib.h>
#include <string>
#include <unordered_map>

class Env {
public:
  static Env &instance() noexcept {
    static Env env;
    return env;
  }

  std::optional<std::string> get(const std::string &key) const noexcept {
    if (auto x = env_.find(key); x != env_.end() && ::getenv(key.c_str()))
      return std::make_optional(x->second);
    return std::nullopt;
  }

  void set(const std::string &key, const std::string &value) {
    env_[key] = value;
    ::setenv(key.c_str(), value.c_str(), true);
  }
  decltype(auto) env() const { return env_; }

private:
  Env() = default;
  ~Env() = default;
  Env(const Env &) = delete;
  Env(Env &&) = delete;
  Env &operator=(const Env &) = delete;
  Env &operator=(Env &&) = delete;

private:
  std::unordered_map<std::string, std::string> env_;
};

#endif
