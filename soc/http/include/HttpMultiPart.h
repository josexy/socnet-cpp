#ifndef SOC_HTTP_HTTPMULTIPART_H
#define SOC_HTTP_HTTPMULTIPART_H

#include "HttpHeader.h"
#include <functional>
#include <unordered_map>

namespace soc {
namespace http {

class HttpMultiPart {
public:
  struct Part {
    std::string name;
    std::string file_name;
    std::string file_type;
    std::string data_str;
  };

  void set_boundary(const std::string &boundary) noexcept { bd_ = boundary; }
  const std::string &boundary() const noexcept { return bd_; }
  std::optional<std::string> get(const std::string &name) const;
  std::optional<const std::reference_wrapper<const std::vector<Part>>>
  file(const std::string &name) const;

  void parse(const std::string_view &body);

private:
  enum State {
    start_body,
    start_boundary,
    end_boundary,
    start_content_disposition,
    end_content_disposition,
    start_content_type,
    end_content_type,
    start_content_data,
    end_content_data,
    end_body
  };

  std::string bd_;
  std::unordered_map<std::string, std::string> form_;
  std::unordered_map<std::string, std::vector<Part>> files_;

  inline constexpr static char CR = '\r';
  inline constexpr static char LF = '\n';
};
} // namespace http
} // namespace soc

#endif
