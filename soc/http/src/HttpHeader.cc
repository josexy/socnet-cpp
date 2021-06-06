#include "../include/HttpHeader.h"

using namespace soc::http;

HttpHeader::HttpHeader(std::string_view header) { parse(header); }

int HttpHeader::parse(std::string_view header) {
  std::string_view line_header;
  int index = 0;
  while (true) {
    if (header.size() >= 2 && header[0] == '\r' && header[1] == '\n') {
      index += 2;
      break;
    }
    size_t pos = header.find("\r\n");
    if (pos == header.npos)
      break;

    line_header = header.substr(0, pos);

    size_t pos2 = line_header.find_first_of(":");
    if (pos2 == line_header.npos)
      break;

    std::string_view key = line_header.substr(0, pos2);
    std::string_view value = line_header.substr(pos2 + 2, pos - pos2 - 2);
    headers_.emplace(std::string(key.data(), key.size()),
                     std::string(value.data(), value.size()));

    header.remove_prefix(pos + 2);
    index += pos + 2;
  }
  return index;
}

void HttpHeader::add(const std::string &key, const std::string &value) {
  if (!key.starts_with("Set-Cookie"))
    if (auto x = headers_.find(key); x != headers_.end()) {
      headers_.lower_bound(key)->second = value;
      return;
    }
  headers_.emplace(std::forward<const std::string &>(key),
                   std::forward<const std::string &>(value));
}

void HttpHeader::remove(const std::string &key) {
  if (headers_.find(key) != headers_.end()) {
    headers_.erase(key);
  }
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
