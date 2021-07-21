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
void FileUtil::readAll(net::Buffer *buffer) {
  if (!fp_)
    return;
  ::fseek(fp_, 0, SEEK_END);
  auto size = ::ftell(fp_);
  ::rewind(fp_);
  buffer->ensureWritable(size);
  ::fread(buffer->begin_write(), sizeof(char), size, fp_);
  buffer->hasWritten(size);
}

bool FileUtil::exist(std::string_view file) {
  struct stat st;
  int ret = ::stat(file.data(), &st);
  return ret != -1 && S_ISREG(st.st_mode);
}
