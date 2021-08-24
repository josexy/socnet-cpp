#include "../include/HttpHeader.h"

using namespace soc::http;

HttpHeader::HttpHeader() {}

HttpHeader::HttpHeader(std::string_view header) {
  WriteLock locker(mutex_);
  parse(header);
}

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
    while (line_header[pos2 + 1] == ' ')
      pos2++;
    pos2++;
    std::string_view value = line_header.substr(pos2, pos - pos2);
    add0(std::string(key.data(), key.size()),
         std::string(value.data(), value.size()));

    header.remove_prefix(pos + 2);
    index += pos + 2;
  }
  return index;
}

void HttpHeader::add(const HttpHeader &header) {
  std::lock(mutex_, header.mutex_);
  WriteLock locker1(mutex_, std::adopt_lock);
  WriteLock locker2(header.mutex_, std::adopt_lock);

  for (const auto &[k, v] : header.headers_) {
    add0(k, v);
  }
}

void HttpHeader::add(const std::string &key, const std::string &value) {
  WriteLock locker(mutex_);
  add0(key, value);
}

void HttpHeader::add0(const std::string &key, const std::string &value) {
  // Save lowercase key
  std::string k = EncodeUtil::toLowers(key);
  if (!k.starts_with("set-cookie")) {
    if (auto x = headers_.find(k); x != headers_.end()) {
      headers_.lower_bound(k)->second = value;
      return;
    }
  }
  headers_.emplace(std::forward<const std::string &>(k),
                   std::forward<const std::string &>(value));
}

void HttpHeader::remove(const std::string &key) {
  WriteLock locker(mutex_);
  std::string k = EncodeUtil::toLowers(key);
  if (headers_.find(k) != headers_.end()) {
    headers_.erase(k);
  }
}

std::optional<std::string> HttpHeader::get(const std::string &key) const {
  ReadLock locker(mutex_);
  std::string k = EncodeUtil::toLowers(key);
  if (auto it = headers_.find(k); it != headers_.end())
    return std::make_optional(it->second);
  return std::nullopt;
}

bool HttpHeader::contain(const std::string &key) const noexcept {
  ReadLock locker(mutex_);
  return headers_.find(EncodeUtil::toLowers(key)) != headers_.end();
}

std::string HttpHeader::toString() const {
  ReadLock locker(mutex_);
  std::string str;
  for (const auto &[key, value] : headers_)
    str += key + ": " + value + "\r\n";
  return str;
}

void HttpHeader::forEach(const Callback &callback) const {
  ReadLock locker(mutex_);
  for (const auto &[key, value] : headers_)
    callback(key, value);
}

void HttpHeader::store(soc::net::Buffer *sender) {
  ReadLock locker(mutex_);
  if (!sender)
    return;
  for (const auto &[key, value] : headers_) {
    std::string v = key + ": " + value + "\r\n";
    sender->append(v.data(), v.size());
  }
}
