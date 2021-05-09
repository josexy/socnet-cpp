#include "../include/HttpHeader.h"

using namespace soc::http;

void HttpHeader::add(const std::string &key, const std::string &value) {
  if (!key.starts_with("Set-Cookie"))
    if (auto x = headers_.find(key); x != headers_.end()) {
      headers_.lower_bound(key)->second = value;
      return;
    }
  headers_.emplace(std::forward<const std::string &>(key),
                   std::forward<const std::string &>(value));
}

std::optional<std::string> HttpHeader::get(const std::string &key) const {
  if (auto it = headers_.find(key); it != headers_.end())
    return std::make_optional(it->second);
  return std::nullopt;
}

std::string HttpHeader::to_string() const {
  std::string str;
  for (auto &[key, value] : headers_)
    str += key + ": " + value + "\r\n";
  return str;
}

void HttpHeader::store(soc::net::Buffer *sender) {
  if (!sender)
    return;
  for (auto &[key, value] : headers_) {
    std::string v = key + ": " + value + "\r\n";
    sender->append(v.data(), v.size());
  }
}
