#ifndef SOC_UTILITY_FILELOADER_H
#define SOC_UTILITY_FILELOADER_H

#include <string>
namespace soc {
class FileLoader {
public:
  FileLoader();
  explicit FileLoader(const std::string_view &file);
  ~FileLoader();

  bool load(const std::string_view &file);
  bool is_open() const { return fp_ != nullptr; }

  bool read_line(std::string &line);
  void read_all(std::string &data);

  static bool exist(const std::string_view &file);
  static bool exist_dir(const std::string_view &dir);

private:
  FILE *fp_;
};
} // namespace soc

#endif
