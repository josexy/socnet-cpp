#ifndef SOC_HTTP_HTTPHEADER_H
#define SOC_HTTP_HTTPHEADER_H

#include <map>
#include <string>

#include "../../net/include/Buffer.h"

namespace soc {
namespace http {

class HttpHeader {
public:
  void add(const std::string &key, const std::string &value);
  std::optional<std::string> get(const std::string &key) const;
  size_t count() const noexcept { return headers_.size(); }

  std::string to_string() const;
  decltype(auto) items() const { return headers_; }

  void store(net::Buffer *sender);

private:
  std::multimap<std::string, std::string> headers_;
};
} // namespace http
} // namespace soc
#endif
