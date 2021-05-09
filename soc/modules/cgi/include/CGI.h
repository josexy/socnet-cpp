#ifndef SOC_CGI_H
#define SOC_CGI_H

#include "../../../http/include/HttpResponse.h"

namespace soc {

// This is a simple cgi processing module
class CGI {
public:
  static bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  }
  static void trim(std::string_view &s) {
    for (int i = 0, j = (int)s.size() - 1; i < j; i++, j--) {
      if (is_space(s[i]))
        s.remove_prefix(1);
      if (is_space(s[j]))
        s.remove_suffix(1);
    }
  }

  static void execute_cgi_script(const std::string_view &script,
                                 http::HttpResponse &resp) {
    FILE *fp = ::popen(script.data(), "r");
    char buffer[1024]{0};
    bool header_flag = true;

    while (::fgets(buffer, 1024, fp)) {
      if ((buffer[0] == '\n') || (buffer[0] == '\r' && buffer[1] == '\n')) {
        header_flag = false;
        continue;
      }

      if (header_flag) {
        std::string_view line(buffer);

        size_t pos = line.find_first_of(":");
        if (pos == std::string_view::npos)
          continue;
        std::string_view key = line.substr(0, pos);
        std::string_view value = line.substr(pos + 1);
        trim(key);
        trim(value);
        resp.header(std::string(key.data(), key.size()),
                    std::string(value.data(), value.size()));
      } else {
        resp.append_body(buffer);
      }
    }
    ::pclose(fp);
  }
};
} // namespace soc
#endif
