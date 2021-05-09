#ifndef LIBJSON_JSONREADER_H
#define LIBJSON_JSONREADER_H

#include <string.h>

#include <fstream>

#include "JsonError.h"

namespace libjson {
class JsonReader {
public:
  JsonReader(const JsonReader &) = delete;
  JsonReader(JsonReader &&) = delete;
  JsonReader &operator=(const JsonReader &) = delete;
  JsonReader &operator=(JsonReader &&) = delete;

  JsonReader() {}
  explicit JsonReader(const std::string &str) { set_buffer(str); }

  explicit JsonReader(std::ifstream &ifs) {
    char c;
    std::string s;
    while (~(c = ifs.get()))
      s += c;
    if (s.empty())
      throw JsonError("Empty json document");
    set_buffer(s);
  }
  ~JsonReader() {
    if (__sbuffer)
      delete[] __sbuffer;
  }
  void set_buffer(const std::string &s) {
    if (__sbuffer) {
      delete[] __sbuffer;
      __sbuffer = nullptr;
    }
    __len = s.length();
    __ptr = -1;
    __sbuffer = new char[__len]{0};
    memcpy(__sbuffer, s.data(), __len * sizeof(char));
  }

  char cur() {
    if (__ptr <= -1 || __ptr >= __len)
      return EOF;
    return __sbuffer[__ptr];
  }
  char get() {
    if (++__ptr >= __len)
      return EOF;
    return __sbuffer[__ptr];
  }
  char backc() {
    if (__ptr - 1 < 0)
      return EOF;
    return __sbuffer[__ptr - 1];
  }
  void back() { --__ptr; }

  char peek() {
    if (__ptr + 1 >= __len)
      return EOF;
    return __sbuffer[__ptr + 1];
  }
  std::pair<bool, char> geteq(int c) {
    char x = get();
    if (x == c)
      return std::make_pair(true, c);
    back();
    return std::make_pair(false, c);
  }
  std::string gets(int count) {
    std::string s;
    for (int i = 0; i < count; i++)
      s.append(1, get());
    return s;
  }
  int len() { return __len; }
  int pos() { return __ptr; }
  bool eof() { return __ptr > __len - 1; }

  void reset() { __ptr = -1; }
  void clear() {
    reset();
    memset(__sbuffer, 0, __len * sizeof(char));
  }

private:
  char *__sbuffer = nullptr;
  int __ptr = -1, __len = 0;
};
} // namespace libjson
#endif
