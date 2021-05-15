#ifndef SOC_UTILITY_FILELOADER_H
#define SOC_UTILITY_FILELOADER_H

#include "../../net/include/Buffer.h"
#include <string.h>
#include <string>
#include <sys/stat.h>

namespace soc {
namespace net {
class Buffer;
} // namespace net

class FileLoader {
public:
  FileLoader();
  explicit FileLoader(const std::string_view &file);
  ~FileLoader();

  bool load(const std::string_view &file);
  bool is_open() const { return fp_ != nullptr; }

  bool read_line(std::string &line);
  void read_all(net::Buffer *buffer);

  static bool exist(const std::string_view &file);
  static bool exist_dir(const std::string_view &dir);

private:
  FILE *fp_;
};
} // namespace soc

#endif
