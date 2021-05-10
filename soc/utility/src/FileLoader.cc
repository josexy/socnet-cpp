#include "../include/FileLoader.h"

#include <string.h>
#include <sys/stat.h>

using namespace soc;

FileLoader::FileLoader() : fp_(nullptr) {}

FileLoader::FileLoader(const std::string_view &file) : fp_(nullptr) {
  load(file);
}

FileLoader::~FileLoader() {
  if (!fp_)
    return;
  ::fclose(fp_);
  fp_ = nullptr;
}

bool FileLoader::load(const std::string_view &file) {
  if (fp_)
    return false;
  fp_ = ::fopen(file.data(), "rb");
  return true;
}

// cannot read binary file
bool FileLoader::read_line(std::string &line) {
  if (!fp_ || ::feof(fp_))
    return false;
  line.resize(1024, 0);
  char *s = ::fgets(line.data(), 1024, fp_);
  if (!s)
    return false;
  char *nl = ::strchr(line.data(), '\n');
  if (nl)
    line.resize(nl - line.data());
  else
    line.resize(::strlen(line.data()));
  return true;
}

// can read binary file or regular file
void FileLoader::read_all(std::string &data) {
  if (!fp_)
    return;
  char buffer[4096] = {0};
  int n;
  while ((n = ::fread(buffer, sizeof(char), 4096, fp_)) > 0)
    data.append(buffer, n);
}

bool FileLoader::exist(const std::string_view &file) {
  struct stat st;
  int ret = ::stat(file.data(), &st);
  return ret != -1 && S_ISREG(st.st_mode);
}

bool FileLoader::exist_dir(const std::string_view &dir) {
  struct stat st;
  int ret = ::stat(dir.data(), &st);
  return ret != -1 && S_ISDIR(st.st_mode);
}