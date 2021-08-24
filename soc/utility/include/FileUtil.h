#ifndef SOC_UTILITY_FILEUTIL_H
#define SOC_UTILITY_FILEUTIL_H

#include <fcntl.h>
#include <filesystem>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

namespace soc {
class FileUtil {
public:
  FileUtil();
  explicit FileUtil(std::string_view file);
  ~FileUtil();

  bool load(std::string_view file);
  bool isOpen() const { return fp_ != nullptr; }
  bool readLine(std::string &line);

  static int openFile(std::string_view file);
  static void closeFile(int fd);
  static int read(int fd, void *data, size_t size);
  static bool exist(std::string_view file);

private:
  FILE *fp_;
};
} // namespace soc

#endif
