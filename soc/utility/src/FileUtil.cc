#include "../include/FileUtil.h"

using namespace soc;

FileUtil::FileUtil() : fp_(nullptr) {}

FileUtil::FileUtil(std::string_view file) : fp_(nullptr) { load(file); }

FileUtil::~FileUtil() {
  if (!fp_)
    return;
  ::fclose(fp_);
  fp_ = nullptr;
}

bool FileUtil::load(std::string_view file) {
  if (fp_)
    return false;
  fp_ = ::fopen(file.data(), "rb");
  return true;
}

// cannot read binary file
bool FileUtil::readLine(std::string &line) {
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

int FileUtil::openFile(std::string_view file) {
  return ::open(file.data(), O_RDONLY);
}

void FileUtil::closeFile(int fd) { ::close(fd); }

int FileUtil::read(int fd, void *data, size_t size) {
  return ::read(fd, data, size);
}

bool FileUtil::exist(std::string_view file) {
  struct stat st;
  int ret = ::stat(file.data(), &st);
  return ret != -1 && S_ISREG(st.st_mode);
}
