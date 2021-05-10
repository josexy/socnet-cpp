
#ifndef SOC_UTILITY_FILELISTER_H
#define SOC_UTILITY_FILELISTER_H

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

namespace soc {
class FileLister {
public:
  struct FileInfo {
    std::string name;
    time_t lastmodtime;
    unsigned long int size;
    bool is_dir;

    FileInfo(const std::string &name, time_t lastmodtime, unsigned int size,
             bool is_dir)
        : name(name), lastmodtime(lastmodtime), size(size), is_dir(is_dir) {}
  };

  static void list(std::string_view dirname, std::vector<FileInfo> &v) {
    if (dirname.empty())
      return;
    if (dirname.back() == '/')
      dirname.remove_suffix(1);

    DIR *dir = ::opendir(dirname.data());
    if (!dir)
      return;
    dirent *entry;
    while ((entry = ::readdir(dir))) {
      if (strncmp(entry->d_name, ".", 1) == 0 ||
          strncmp(entry->d_name, "..", 2) == 0)
        continue;
      std::string filename = std::string(dirname.data(), dirname.size()) +
                             +"/" + std::string(entry->d_name);
      struct stat st;
      if (-1 != ::stat(filename.c_str(), &st))
        v.emplace_back(entry->d_name, st.st_mtim.tv_sec, st.st_size,
                       entry->d_type == DT_DIR);
    }
    ::closedir(dir);
  } // namespace soc
};  // namespace soc

} // namespace soc

#endif
