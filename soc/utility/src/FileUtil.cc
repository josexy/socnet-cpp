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

// can read binary file or regular file
std::string_view FileUtil::readAll(net::Buffer *buffer) {
  if (!fp_)
    return 0;

  int fd = ::fileno(fp_);
  struct stat st;
  ::fstat(fd, &st);

  constexpr static size_t M = 2 * 1024 * 1024;

  char *svbuf = nullptr;
  if (st.st_size <= M) {
    buffer->ensureWritable(st.st_size);
    ::fread(buffer->beginWrite(), sizeof(char), st.st_size, fp_);
    buffer->hasWritten(st.st_size);
    svbuf = buffer->beginRead();
  } else {
    char *mapping = static_cast<char *>(
        ::mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (mapping == MAP_FAILED)
      return 0;
    buffer->appendMapping(mapping, st.st_size);
    svbuf = buffer->mappingAddr();
  }
  return std::string_view(svbuf, st.st_size);
}

bool FileUtil::exist(std::string_view file) {
  struct stat st;
  int ret = ::stat(file.data(), &st);
  return ret != -1 && S_ISREG(st.st_mode);
}
