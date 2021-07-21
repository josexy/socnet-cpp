#ifndef SOC_UTILITY_FILEUTIL_H
#define SOC_UTILITY_FILEUTIL_H

#include "../../net/include/Buffer.h"
#include <filesystem>
#include <string.h>
#include <sys/stat.h>

namespace soc {
class FileUtil {
public:
  FileUtil();
  explicit FileUtil(std::string_view file);
  ~FileUtil();

  bool load(std::string_view file);
  bool isOpen() const { return fp_ != nullptr; }

  bool readLine(std::string &line);
  void readAll(net::Buffer *buffer);

  static bool exist(std::string_view file);

private:
  FILE *fp_;
};
} // namespace soc

#endif
