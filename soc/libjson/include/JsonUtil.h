#ifndef LIBJSON_JSONUTIL_H
#define LIBJSON_JSONUTIL_H

#include <bitset>
#include <codecvt>
#include <locale>

#include "JsonError.h"

namespace libjson {
// Convert hexadecimal, octal, and binary to decimal
template <class T, size_t Base> inline T toBase(const std::string &s) {
  if (Base == 2)
    return std::bitset<32>(s).to_ulong();

  std::stringstream ss;
  T val;
  if constexpr (Base == 16)
    ss << std::hex;
  else if constexpr (Base == 8)
    ss << std::oct;
  ss << s;
  ss >> val;
  return val;
}

template <class T> inline std::string toStr(const T &val) {
  std::stringstream ss;
  std::string s;
  ss << val;
  ss >> s;
  return s;
}

template <class T> inline T toAny(const std::string &s) {
  std::stringstream ss;
  ss << s;
  T val;
  ss >> val;
  return val;
}

// Convert hexadecimal unicode to text
inline std::string unicode_hex_to_string(const std::string &unicode) {
  int val = toBase<int, 16>(unicode);
  std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
  return convert.to_bytes(val);
}

inline bool isletter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
inline bool isnumber(char c) { return c >= '0' && c <= '9'; }
inline bool ishex(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}
inline bool isoct(char c) { return c >= '0' && c <= '7'; }
inline bool isbin(char c) { return c == '0' || c == '1'; }
inline bool isexponent(char c) { return c == 'e' || c == 'E'; }

inline bool isspace(char c) { return c == ' ' || c == '\t' || c == '\v'; }
inline bool isnewline(char c) { return c == '\n'; }

inline bool is_objectbegin(char c) { return c == '{'; }
inline bool is_objectend(char c) { return c == '}'; }
inline bool is_arraybegin(char c) { return c == '['; }
inline bool is_arrayend(char c) { return c == ']'; }
inline bool is_string_begin_end(char c) { return c == '"'; }

inline bool is_valid_next_escape_character(char c) {
  switch (c) {
  case 't':
  case 'n':
  case '0':
  case 'b':
  case 'f':
  case 'x':
  case 'u':
  case 'r':
  case '\"':
  case '\\':
    return true;
  default:
    break;
  }
  return false;
}

inline std::string unescape_character(char c) {
  std::string ec(1, c);
  switch (c) {
  case '\n':
    ec = "\\n";
    break;
  case '\0':
    ec = "\\0";
    break;
  case '\t':
    ec = "\\t";
    break;
  case '\"':
    ec = "\\\"";
    break;
  case '\b':
    ec = "\\b";
    break;
  case '\f':
    ec = "\\f";
    break;
  case '\r':
    ec = "\\r";
    break;
  case '\\':
    ec = "\\\\";
    break;
  default:
    break;
  }
  return ec;
}

inline char escape_character(char c) {
  char ec = c;
  switch (c) {
  case 'n':
    ec = '\n';
    break;
  case '0':
    ec = '\0';
    break;
  case 't':
    ec = '\t';
    break;
  case 'b':
    ec = '\b';
    break;
  case 'f':
    ec = '\f';
    break;
  case 'r':
    ec = '\r';
    break;
  case '\\':
    ec = '\\';
    break;
  default:
    break;
  }
  return ec;
}

// Escape special characters,such as parse the string "\t" as a tab character \t
inline std::string escape_string(const std::string &s) {
  std::string __escape_s;
  for (size_t i = 0; i < s.size(); i++) {
    char c = s[i];
    if (c == '\\') {
      c = s[++i];
      switch (c) {
      case 'u': {
        std::string hexuni;
        c = s[++i];
        for (int j = 0; j < 4; j++, c = s[++i]) {
          if (ishex(c))
            hexuni.append(1, c);
          else
            throw JsonError("need 4 hexadecimal digits after \\u");
        }

        --i;
        __escape_s.append(std::move(unicode_hex_to_string(hexuni)));
      } break;
      case 'x': {
        std::string hexs;
        c = s[++i];
        while (ishex(c)) {
          hexs.append(1, c);
          c = s[++i];
        }
        if (hexs.empty())
          throw JsonError("need hexadecimal digits after \\x");
        --i;
        __escape_s.append(std::move(std::to_string(toBase<int, 16>(hexs))));
      } break;
      default:
        __escape_s.append(1, std::move(escape_character(c)));
        break;
      }
    } else {
      __escape_s.append(1, c);
    }
  }
  return __escape_s;
}
// Reverse special characters, such as parse '\t' as a "\\t" string
inline std::string unescape_string(const std::string &s) {
  std::string __unescape_s;
  for (size_t i = 0; i < s.size(); i++)
    __unescape_s.append(std::move(unescape_character(s[i])));
  return __unescape_s;
}
} // namespace libjson
#endif
