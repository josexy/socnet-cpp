#include "../include/HttpCookie.h"
using namespace soc::http;

std::string HttpCookie::toString() const {
  ReadLock locker(mutex_);
  std::string str;
  size_t i = 0, n = size();
  for (const auto &[key, value] : map_) {
    str += key + "=" + value;
    i++;
    if (i < n)
      str += "; ";
  }
  if (!expires_.empty())
    str += "; Expires=" + expires_;
  if (!domain_.empty())
    str += "; Domain=" + domain_;
  if (!path_.empty())
    str += "; Path=" + path_;
  if (max_age_ > 0)
    str += "; Max-Age=" + std::to_string(max_age_);
  if (secure_)
    str += "; Secure";
  if (http_only_)
    str += "; HttpOnly";
  return str;
}